# GBuffer 入门 - 程序员版

本文面向有 C++ 基础、但尚未系统学习实时渲染的人。重点不是做概念类比，而是回答下面几个更具体的问题：

- `GBuffer` 在图形管线里到底是什么资源
- 它在 UE 里分别对应哪些类型和变量
- `FGBufferData`、`SceneTexturesStruct`、`GBufferA/B/C...` 之间是什么关系
- 为什么法线可视化可以直接从 `GBuffer` 读取，而不是重新计算

## 1. 定义

`GBuffer` 不是一个单独的变量，也不是一个 C++ 结构体。

在延迟渲染语境下，`GBuffer` 的本质是：

- 一组 GPU render target / texture 资源
- 这些纹理以“每个屏幕像素一份”的方式存储几何表面属性
- 后续光照和屏幕空间效果通过读取这些纹理，而不是重新遍历材质输入

因此，`GBuffer` 是“屏幕空间表面属性缓存”，而不是“最终颜色缓存”。

## 2. 从底层看，它到底存的是什么

先纠正一个常见误解：现代渲染里的像素通常不能简单理解成“一个 `int` 存颜色”。

在 GPU 上，更准确的说法是：

- 一个 render target 是一张纹理
- 纹理由许多 texel 组成
- 每个 texel 的存储格式由像素格式决定，例如 `R8G8B8A8`、`R16G16B16A16F` 等

所以“一个像素是什么”取决于当前 attachment 的格式，而不是一个固定的 CPU 结构体。

对 `GBuffer` 来说：

- 它通常由多张纹理组成，而不是一张
- 每张纹理保存一部分属性
- 属性会被打包、量化、编码

这也是为什么你在 shader 里不会直接写：

```cpp
struct Pixel
{
    float3 Normal;
    float Roughness;
    float Metallic;
};
```

然后把它整块读出来。

实际情况更接近：

- `GBufferA` 存部分属性
- `GBufferB` 存另一部分属性
- `GBufferC/D/E/F` 再补充剩余属性
- shader 端再把这些分散的通道解码成逻辑上的“表面数据结构”

## 3. UE 里，GBuffer 分别落在哪几层

在 UE 当前这套代码里，`GBuffer` 至少可以从三个层次去看。

### 3.1 C++ / RDG 层：它是“要绑定哪些场景纹理”的标志集合

文件：
[SceneRenderTargetParameters.h](/E:/uey/UnrealEngine-5.5/UnrealEngine-5.5/Engine/Source/Runtime/Renderer/Public/SceneRenderTargetParameters.h)

这里有：

```cpp
enum class ESceneTextureSetupMode : uint32
{
    ...
    GBufferA = 1 << 3,
    GBufferB = 1 << 4,
    GBufferC = 1 << 5,
    GBufferD = 1 << 6,
    GBufferE = 1 << 7,
    GBufferF = 1 << 8,
    ...
    GBuffers = GBufferA | GBufferB | GBufferC | GBufferD | GBufferE | GBufferF,
};
```

这里的 `GBuffers` 只是一个枚举位掩码，不是实际数据。

它表达的是：

- 当前 pass 需要访问 `GBufferA` 到 `GBufferF`
- 渲染器需要把这些纹理放进后续 shader 可访问的参数里

你当前插件里写的是：

```cpp
PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
    GraphBuilder,
    View,
    ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
```

这句的含义是：

- 生成一个包含指定场景纹理引用的 uniform buffer
- 其中包括深度和 GBuffer
- 后面的 shader 会通过这个 uniform buffer 访问这些纹理

### 3.2 Shader 绑定层：它是一个 uniform buffer

同一个头文件里还有：

```cpp
TRDGUniformBufferRef<FSceneTextureUniformParameters> CreateSceneTextureUniformBuffer(...)
```

这说明从 C++ 传到 shader 的，不是若干裸纹理指针，而是一个 `FSceneTextureUniformParameters` uniform buffer。

在你的 shader 参数结构里，对应的是：

```cpp
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
```

也就是说：

- `SceneTextures` 是一个 RDG uniform buffer 参数
- 它内部再包含 `SceneDepthTexture`、`GBufferATexture` 等具体纹理引用

### 3.3 HLSL 层：它表现为 `SceneTexturesStruct`

文件：
[SceneTexturesCommon.ush](/E:/uey/UnrealEngine-5.5/UnrealEngine-5.5/Engine/Shaders/Private/SceneTexturesCommon.ush)

这里可以看到类似内容：

```hlsl
#define SceneTexturesStruct_GBufferATextureSampler SceneTexturesStruct.PointClampSampler
...
return Texture2DSampleLevel(SceneTexturesStruct.SceneDepthTexture, ...);
```

这说明在 shader 里，`FSceneTextureUniformParameters` 会以 `SceneTexturesStruct` 的形式出现。

因此，从 HLSL 视角看：

- `SceneTexturesStruct` 是“已绑定的场景纹理集合”
- `SceneTexturesStruct.GBufferATexture` 是其中一张具体纹理
- `SceneTexturesStruct.SceneDepthTexture` 是深度纹理

这才是 shader 真正采样的数据源。

## 4. `FGBufferData` 是什么

这也是最容易混淆的地方。

文件：
[DeferredShadingCommon.ush](/E:/uey/UnrealEngine-5.5/UnrealEngine-5.5/Engine/Shaders/Private/DeferredShadingCommon.ush)

里面有：

```hlsl
struct FGBufferData
{
    half3 WorldNormal;
    half3 WorldTangent;
    half3 DiffuseColor;
    half3 SpecularColor;
    half3 BaseColor;
    half Metallic;
    half Specular;
    half4 CustomData;
    ...
};
```

这里的 `FGBufferData` 不是物理上的 `GBuffer`。

它是：

- shader 端的逻辑结构体
- 表示“把多张 GBuffer 纹理解码以后，得到的一份表面属性快照”

换句话说：

- `GBufferA/B/C...` 是物理存储
- `FGBufferData` 是逻辑视图

这两者不是同一个层面的东西。

## 5. `GetGBufferData(UV)` 实际做了什么

同样在 [DeferredShadingCommon.ush](/E:/uey/UnrealEngine-5.5/UnrealEngine-5.5/Engine/Shaders/Private/DeferredShadingCommon.ush) 里，可以看到：

```hlsl
FGBufferData GetGBufferData(float2 UV, bool bGetNormalizedNormal = true)
{
    float4 GBufferA = Texture2DSampleLevel(SceneTexturesStruct.GBufferATexture, ...);
    float4 GBufferB = Texture2DSampleLevel(SceneTexturesStruct.GBufferBTexture, ...);
    float4 GBufferC = Texture2DSampleLevel(SceneTexturesStruct.GBufferCTexture, ...);
    float4 GBufferD = Texture2DSampleLevel(SceneTexturesStruct.GBufferDTexture, ...);
    ...
    float SceneDepth = CalcSceneDepth(UV);

    return DecodeGBufferData(...);
}
```

这段逻辑可以拆成三步：

1. 从 `SceneTexturesStruct` 采样多张 GBuffer 纹理
2. 必要时再读取深度
3. 调用 `DecodeGBufferData(...)` 还原出统一的 `FGBufferData`

所以：

```hlsl
const FGBufferData GBufferData = GetGBufferData(UV);
```

不是“访问一个变量”这么简单。

它底层做的是：

- 多纹理采样
- 编码值解包
- 构造一份逻辑表面数据

## 6. 为什么法线在 GBuffer 里，而不是实时重建

法线是后续计算的核心输入之一。至少下面几类计算都依赖它：

- deferred lighting
- specular / BRDF 计算
- SSAO
- edge detection
- 屏幕空间反射或相关调试视图

如果后续每个 pass 都重新从几何或材质输入推导法线，成本会很高，也不方便复用。

因此更常见的策略是：

- base pass 阶段把法线编码写入 GBuffer
- 后续 pass 直接读取并解码

这也是你当前法线可视化能成立的根本原因。

## 7. 你的法线可视化代码，对应到哪一层

你当前插件文件：
[RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)

这里做的是“绑定资源”：

```cpp
PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
    GraphBuilder,
    View,
    ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
```

这一步还没有读法线，只是把“后面会用到的场景纹理集合”挂到 shader 参数上。

然后在：
[RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf)

你写的是：

```hlsl
const FGBufferData GBufferData = GetGBufferData(UV);
const float3 WorldNormal = normalize(GBufferData.WorldNormal);
const float3 NormalColor = WorldNormal * 0.5f + 0.5f;
```

这一步才是：

- 从 GBuffer 纹理中解码
- 取得逻辑上的世界空间法线
- 再把它映射成屏幕颜色

因此整条链可以精确写成：

1. `ESceneTextureSetupMode::GBuffers`
   表示“本 pass 需要 GBuffer 资源”
2. `FSceneTextureUniformParameters`
   表示“这些资源被打包成 uniform buffer”
3. `SceneTexturesStruct.GBufferATexture`
   表示“shader 端实际可访问的物理纹理”
4. `FGBufferData`
   表示“把物理纹理解码后的逻辑表面数据”
5. `WorldNormal`
   表示“逻辑表面数据中的法线字段”

## 8. GBuffer 与深度的关系

深度缓冲和 GBuffer 不是一回事。

区别如下：

- 深度缓冲主要保存几何可见性的深度值
- GBuffer 主要保存表面属性

但在实际 shader 里两者经常一起使用。

原因是：

- 解码某些表面信息可能依赖深度
- 屏幕空间效果通常同时需要“距离信息”和“表面方向信息”

你当前代码里同时请求：

```cpp
ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers
```

就是因为这两个数据集合通常是一起工作的。

## 9. 常见误区

### 9.1 “GBuffer 就是一个变量”

错误。

更准确的说法是：

- 在概念层，它是一组屏幕空间表面缓存
- 在 C++ 层，它体现为 `ESceneTextureSetupMode::GBuffers`
- 在 shader 资源层，它体现为 `SceneTexturesStruct.GBufferATexture` 等纹理引用

### 9.2 “FGBufferData 就是 GBuffer”

错误。

`FGBufferData` 是 decode 之后的逻辑结构，不是原始存储。

### 9.3 “像素就是一个 int”

只在最简单的颜色缓冲类比里勉强成立。

现代 GPU 渲染中，更准确的说法是：

- 像素输出写入某个 render target
- render target 的 texel 以指定格式存储
- 这个格式可以是 UNorm、SNorm、float、half、packed channels 等

对 GBuffer 来说，属性往往是编码后分散写入多张纹理的。

## 10. 当前阶段最应该记住的结论

如果你现在只记三句话，应该记这三句：

1. `GBuffer` 的本质不是一个结构体，而是一组 GPU 纹理资源。
2. `FGBufferData` 不是存储本体，而是 shader 端解码后的逻辑表面数据。
3. 你的法线可视化不是“重新算法线”，而是“从 GBuffer 读取并解码法线”。

## 11. 下一步最合适的延伸

基于这篇文档，最自然的下一篇是：

- 延迟渲染是什么，GBuffer 在整条渲染流程里位于哪一阶段

再往下就是：

- `GetGBufferData(UV)` 里的 `DecodeGBufferData(...)` 到底如何还原法线和材质参数
