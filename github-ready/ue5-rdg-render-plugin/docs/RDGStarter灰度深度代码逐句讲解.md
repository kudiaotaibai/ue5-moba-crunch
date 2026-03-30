# RDGStarter 灰度深度代码逐句讲解

## 这篇文档讲什么

这篇只讲当前已经跑通的“灰度深度图”版本，也就是下面这两份代码：

- `Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp`
- `Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf`

目标不是讲 SSAO，也不是讲完整渲染理论，而是讲清楚：

- 这版代码到底是怎么把深度图画出来的
- 每个宏是什么意思
- 每个参数是从哪里来的
- 为什么这几行代码一组合，就能把最终画面覆盖成灰度图

## 先用一句话概括整条链

整条链可以压缩成一句话：

`UE 每帧渲染到 Tonemap 后 -> 调到我们的 SceneViewExtension -> 我们往 RDG 里加一个全屏 Pass -> Pixel Shader 读取 SceneDepth -> 转成灰度 -> 输出到屏幕`

如果你想用更程序一点的说法，可以写成：

`SubscribeToPostProcessingPass -> PostProcessPassAfterTonemap_RenderThread -> AddDrawScreenPass -> MainPS -> CalcSceneDepth -> 灰度输出`

## 先看 C++ 文件在干什么

核心文件是：

`Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp`

### 第 1 步：声明一个全局像素着色器类

代码起点是：

```cpp
class FRDGStarterDepthPS : public FGlobalShader
```

这里的意思是：

- `FRDGStarterDepthPS` 是一个 **全局 Shader 类**
- 它不是材质系统里的 Shader
- 它是我们自己通过 C++ 明确声明、明确绑定到 `.usf` 文件的 Shader

你可以先把它理解成：

“C++ 里对一个像素着色器的描述对象”

它本身不是 GPU 代码，真正的 GPU 代码在 `.usf` 里。

### 第 2 步：`DECLARE_GLOBAL_SHADER`

```cpp
DECLARE_GLOBAL_SHADER(FRDGStarterDepthPS);
```

这个宏可以先粗暴理解成：

“告诉 UE：这是一个全局 Shader 类型，请帮我把它注册到全局 Shader 系统里。”

如果没有这个宏：

- UE 不知道这是个可编译、可查找、可实例化的 Shader 类型
- 后面 `TShaderMapRef<FRDGStarterDepthPS>` 也没法正常工作

你现在先不用深究它展开后的模板细节，只要记住：

- `DECLARE_GLOBAL_SHADER` 是 **声明**
- `IMPLEMENT_GLOBAL_SHADER` 是 **绑定实现**

## 第 3 步：`SHADER_USE_PARAMETER_STRUCT`

```cpp
SHADER_USE_PARAMETER_STRUCT(FRDGStarterDepthPS, FGlobalShader);
```

这个宏的作用是：

- 告诉 UE，这个 Shader 使用“参数结构体”方式传参数
- 而不是旧式一个个 `Bind()` 参数的写法

你可以把它理解成：

“这个 Shader 的所有输入参数，统一打包进一个 `FParameters` 结构体里传进去。”

这也是现在 UE 比较主流的写法，优点是：

- 参数集中
- 可读性更好
- 更符合 RDG 的资源管理方式

## 第 4 步：定义 `FParameters`

这段最关键：

```cpp
BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
    RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()
```

这段代码的意思是：

“这个像素着色器运行时，需要从 C++ 拿到哪些输入。”

下面逐个看。

### `BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )`

这表示开始定义一个 Shader 参数结构体，名字叫 `FParameters`。

后面所有 `SHADER_PARAMETER_...` 宏，都是这个结构体里的成员。

你可以把它想成：

```cpp
struct FParameters
{
    ...
};
```

只是 UE 用宏包了一层，方便做反射和参数绑定。

### `SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)`

这一行的作用是：

- 给 Shader 传入当前视图的统一参数
- 名字叫 `View`

这里面的内容非常多，比如：

- 视图矩阵
- 投影矩阵
- 视口大小
- 深度重建参数
- `InvDeviceZToWorldZTransform`

为什么这里必须传 `View`？

因为你在 shader 里最终调用了：

```hlsl
CalcSceneDepth(PixelPos)
```

而 `CalcSceneDepth` 内部会用到 `ConvertFromDeviceZ`，而 `ConvertFromDeviceZ` 又依赖 `View` 里的参数来把深度缓冲里的 `DeviceZ` 还原成线性深度。

所以：

- 没有 `View`
- 就不能正确把深度纹理里的值还原成真实线性深度

### `SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)`

这一行的作用是：

- 给 Shader 传入一整套场景纹理 uniform buffer
- 名字叫 `SceneTextures`

这里面最重要的是：

- `SceneDepthTexture`

但它不只包含深度，还可能包含：

- SceneColor
- GBuffer
- Velocity
- CustomDepth

为什么这里不用“单独传一张深度纹理”，而要传 uniform buffer？

因为这是 UE 更原生、更稳的方式。

你这次前面踩坑的一个关键点，就是一开始自己拼 scene texture 参数，最后运行时 PSO/shader 出问题。改成这个以后，就和引擎自己的很多 pass 更一致了。

### `RENDER_TARGET_BINDING_SLOTS()`

这一行非常重要。

它的意思是：

- 这个 Shader 会往渲染目标写结果
- UE 需要给这个参数结构体补上“RenderTarget 绑定槽”

没有它的话，你后面这句就没地方写：

```cpp
PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
```

你可以把它理解成：

“我要输出颜色，请给我一个能绑定输出纹理的位置。”

## 第 5 步：`ShouldCompilePermutation`

```cpp
static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
    return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}
```

这段的意思是：

- 只有支持 `SM5` 的平台才编这个 Shader

为什么要这样写？

因为这版代码默认目标就是桌面级渲染路径，先不考虑更低特性级别。

你可以把它理解成一层编译过滤：

- 平台不满足条件
- 这个 Shader 就不编

## 第 6 步：`IMPLEMENT_GLOBAL_SHADER`

```cpp
IMPLEMENT_GLOBAL_SHADER(
    FRDGStarterDepthPS,
    "/RDGStarter/Private/RDGStarterFullscreen.usf",
    "MainPS",
    SF_Pixel);
```

这是把 C++ 类型和 `.usf` 里的实际函数绑起来。

逐个看：

- `FRDGStarterDepthPS`
  C++ 里的 Shader 类型

- `"/RDGStarter/Private/RDGStarterFullscreen.usf"`
  Shader 源文件路径

- `"MainPS"`
  `.usf` 里真正要编的入口函数名

- `SF_Pixel`
  表示这是一个 Pixel Shader

这行可以理解成：

“`FRDGStarterDepthPS` 这个 C++ 类型，对应的 GPU 入口函数，就是 `RDGStarterFullscreen.usf` 里的 `MainPS`。”

## 第 7 步：订阅后处理阶段

这段代码是这版真正进入渲染链的入口：

```cpp
void FRDGStarterSceneViewExtension::SubscribeToPostProcessingPass(...)
{
    if ((Pass == EPostProcessingPass::Tonemap || Pass == EPostProcessingPass::FXAA) && bIsPassEnabled)
    {
        InOutPassCallbacks.Add(
            FAfterPassCallbackDelegate::CreateRaw(this, &FRDGStarterSceneViewExtension::PostProcessPassAfterTonemap_RenderThread));
    }
}
```

它的意思是：

- UE 每帧做后处理时，会问你的扩展：你想不想挂到某个后处理 pass 后面？
- 如果当前 pass 是 `Tonemap` 或 `FXAA`，并且这个 pass 当前启用了
- 那我们就注册一个回调

这个回调就是：

```cpp
PostProcessPassAfterTonemap_RenderThread
```

它会在渲染线程上被调用。

为什么这里优先用 `Tonemap`？

因为：

- 它很靠近最终画面
- 你最容易直接看到效果

为什么又兼容 `FXAA`？

因为有些情况下后处理链路不完全一样，留一个更稳的兜底。

## 第 8 步：`IsActiveThisFrame_Internal`

```cpp
bool FRDGStarterSceneViewExtension::IsActiveThisFrame_Internal(...) const
{
    return true;
}
```

这表示：

- 这个扩展每一帧都启用

后面如果你想加开关，可以在这里做：

- 比如读一个 CVar
- 或者只在某些世界里启用

现在返回 `true` 是最简单版本。

## 第 9 步：真正往 RDG 里加 Pass

核心函数是：

```cpp
FScreenPassTexture FRDGStarterSceneViewExtension::PostProcessPassAfterTonemap_RenderThread(...)
```

这就是你的 RDG pass 真正被创建的地方。

下面逐句看。

### `FScreenPassTexture::CopyFromSlice`

```cpp
const FScreenPassTexture SceneColor =
    FScreenPassTexture::CopyFromSlice(
        GraphBuilder,
        Inputs.GetInput(EPostProcessMaterialInput::SceneColor),
        Inputs.OverrideOutput);
```

这句的意思是：

- 从当前后处理输入里，拿到当前的 `SceneColor`
- 把它变成一个当前 pass 能稳定使用的 `FScreenPassTexture`

为什么这里不是直接用 `Inputs.GetInput(...)`？

因为输入给到你的可能是：

- 一个 slice
- 或者有 override output

`CopyFromSlice` 做的事就是：

- 帮你整理成更适合当前 screen pass 使用的形式

你先把它理解成：

“拿到当前阶段的画面纹理，并整理成当前 pass 可直接使用的对象。”

### `FScreenPassRenderTarget Output`

```cpp
FScreenPassRenderTarget Output(SceneColor, ERenderTargetLoadAction::ENoAction);
```

这句的意思是：

- 基于 `SceneColor` 构造一个输出目标
- 我们后面的 pass 会把结果写到这里

`ENoAction` 在这里你可以先粗略理解为：

- 不依赖旧内容
- 这次 pass 会直接覆盖

### `FScreenPassTextureViewport OutputViewport`

```cpp
const FScreenPassTextureViewport OutputViewport(Output);
```

这个对象描述的是：

- 输出纹理的尺寸
- 输出矩形
- 视口范围

后面 `AddDrawScreenPass` 需要它来知道“到底往哪块区域画”。

### 分配参数结构体

```cpp
FRDGStarterDepthPS::FParameters* PassParameters =
    GraphBuilder.AllocParameters<FRDGStarterDepthPS::FParameters>();
```

这句的意思是：

- 在 RDG 里分配一份当前 pass 用的 Shader 参数

你可以理解成：

- 不是普通 `new`
- 而是交给 RDG 管理生命周期

### 填 `View`

```cpp
PassParameters->View = View.ViewUniformBuffer;
```

这里是把当前视图的 uniform buffer 传给 shader。

前面说过，它最重要的作用是：

- 给 `CalcSceneDepth` / `ConvertFromDeviceZ` 提供深度重建所需的视图参数

### 填 `SceneTextures`

```cpp
PassParameters->SceneTextures =
    CreateSceneTextureUniformBuffer(GraphBuilder, View, ESceneTextureSetupMode::SceneDepth);
```

这句非常关键。

它的意思是：

- 根据当前 `View`
- 创建一个场景纹理 uniform buffer
- 这里只要求它准备 `SceneDepth`

`ESceneTextureSetupMode::SceneDepth` 的意思就是：

- 我们这版只需要深度，不需要整套 GBuffer

这是一个很好的习惯：

- 只申请自己真需要的场景纹理
- 保持当前 pass 足够简单

### 绑定输出目标

```cpp
PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
```

这句的意思是：

- 把刚才的 `Output` 绑定成当前像素着色器的 0 号渲染目标

也就是：

- shader 最终 `return` 的颜色，会写到这里

### `TShaderMapRef`

```cpp
TShaderMapRef<FRDGStarterDepthPS> PixelShader(ShaderMap);
```

这句可以理解成：

- 从当前平台/特性级别对应的 shader map 里
- 找到我们这个像素着色器的已编译版本

你可以把它想成：

“拿到这次真正要跑的 GPU 版 `FRDGStarterDepthPS`。”

### `AddDrawScreenPass`

```cpp
AddDrawScreenPass(
    GraphBuilder,
    RDG_EVENT_NAME("RDGStarter.DepthVisualization"),
    FScreenPassViewInfo(View),
    OutputViewport,
    OutputViewport,
    PixelShader,
    PassParameters);
```

这是整个 C++ 里最关键的一句。

它的作用就是：

- 往 RDG 里加一个全屏绘制 pass
- 使用我们指定的 `PixelShader`
- 用 `PassParameters` 作为输入
- 把结果写到 `OutputViewport`

你可以把它理解成：

“在当前画面上跑一遍全屏像素着色器。”

为什么只传了 Pixel Shader，没有显式传 Vertex Shader？

因为 `AddDrawScreenPass` 会使用 UE 默认的全屏 screen pass 顶点着色器。

这也是你现在能少写很多样板代码的原因。

### 为什么最后 `return SceneColor`

```cpp
return SceneColor;
```

这里很多新手会困惑：

“我不是已经写到 `Output` 了吗，为什么还返回 `SceneColor`？”

原因是：

- 这里的 `SceneColor` 本身就是当前 pass 输出链上的那张屏幕纹理对象
- 你已经把 `Output` 绑定到它上面了
- pass 执行后，这个对象代表的就是“被你改写后的当前场景颜色”

所以这里返回 `SceneColor`，是在把“修改后的画面”继续传给后面的后处理链。

## 再看 `.usf` 文件在干什么

文件是：

`Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf`

### 第 1 行：`#include "/Engine/Private/SceneTexturesCommon.ush"`

这一行非常关键。

它给你带来了几样非常重要的现成能力：

- `SceneTexturesStruct`
- `CalcSceneDepth(...)`
- `LookupDeviceZ(...)`
- 各种 SceneTexture 的采样辅助函数

你这次之所以能把 shader 写得这么短，核心原因就是：

- 没有自己手写 scene texture 结构
- 直接复用了引擎已有工具

### `MainPS` 的函数签名

```hlsl
float4 MainPS(
    noperspective float4 UVAndScreenPos : TEXCOORD0,
    float4 SvPosition : SV_POSITION) : SV_Target0
```

逐个解释。

#### `float4 MainPS`

表示：

- 这是一个像素着色器入口函数
- 返回值是一个 `float4`
- 最终会写到颜色输出

#### `noperspective float4 UVAndScreenPos : TEXCOORD0`

这个参数来自 screen pass 顶点着色器传下来的插值数据。

现在这版代码里其实没用到它，但保留它是正常的，因为：

- 这是全屏 screen pass 常见的标准输入之一
- 后面如果你要按 UV 采样，就会用到它

`noperspective` 可以先简单理解成：

- 插值时不做透视校正

对全屏后处理 pass 来说，这通常是合理的。

#### `float4 SvPosition : SV_POSITION`

这个是当前像素的屏幕位置。

你这里真正用它来做了：

```hlsl
const uint2 PixelPos = uint2(SvPosition.xy);
```

也就是：

- 取当前像素在屏幕上的整数坐标

#### `: SV_Target0`

表示：

- 这个函数返回的颜色写到 0 号渲染目标

它和 C++ 里：

```cpp
PassParameters->RenderTargets[0] = ...
```

正好对应。

## Shader 主体到底干了什么

### 第 1 句：把屏幕坐标转成像素坐标

```hlsl
const uint2 PixelPos = uint2(SvPosition.xy);
```

这句的意思很简单：

- 当前像素在屏幕上的位置
- 转成整数像素坐标

比如：

- `(100.3, 250.7)` 会被转成 `(100, 250)`

### 第 2 句：读取线性深度

```hlsl
const float LinearDepth = CalcSceneDepth(PixelPos);
```

这是这版 shader 的核心。

它做了两件事：

1. 从场景深度纹理里读当前像素的深度值
2. 把深度缓冲里的 `DeviceZ` 转成线性深度

你可以理解成：

- 深度纹理原始存的不是“真实距离”
- 而是 GPU 深度缓冲格式下的值
- `CalcSceneDepth` 帮你做了恢复

所以现在你拿到的 `LinearDepth`，已经比直接读 `SceneDepthTexture` 更适合做可视化。

### 第 3 句：把深度映射到 0~1 灰度

```hlsl
const float DepthGray = saturate(LinearDepth / 10000.0f);
```

这句是“为什么现在画面会变成黑白”的关键。

先拆开看：

#### `LinearDepth / 10000.0f`

意思是：

- 假设 `10000cm` 作为可视化上限
- 那么：
  - `0cm -> 0`
  - `5000cm -> 0.5`
  - `10000cm -> 1`
  - 更远的会大于 1

#### `saturate(...)`

`saturate(x)` 的意思是：

- 把结果夹到 `[0, 1]`

也就是：

- 小于 0 变成 0
- 大于 1 变成 1

所以这句完整的含义是：

- 把线性深度压到 0~1
- 然后拿这个值当灰度强度

为什么你现在画面偏白？

因为当前场景很大，而 `10000cm` 这个范围对它来说偏小：

- 远处山体和天空很容易饱和到 1
- 于是大片区域接近白色

这不是 bug，只是当前可视化映射比较粗糙。

### 第 4 句：输出灰度颜色

```hlsl
return float4(DepthGray.xxx, 1.0f);
```

这里的 `DepthGray.xxx` 是一个常见 HLSL 写法。

意思是：

- 把一个标量复制成三个分量

也就是：

- `float3(DepthGray, DepthGray, DepthGray)`

所以最后返回的是：

- `R = DepthGray`
- `G = DepthGray`
- `B = DepthGray`
- `A = 1.0`

这就是标准灰度图输出。

## 为什么近处黑、远处白

因为你现在的映射是：

- 深度越小
- `DepthGray` 越接近 0
- 越黑

- 深度越大
- `DepthGray` 越接近 1
- 越白

所以：

- 近处物体更暗
- 远处物体更亮

## 这版代码最值得记住的 6 个点

### 1. `DECLARE_GLOBAL_SHADER` / `IMPLEMENT_GLOBAL_SHADER`

它们负责把 C++ 里的 shader 类型和 `.usf` 里的入口函数连接起来。

### 2. `FParameters`

它定义了 shader 运行时到底需要什么输入。

### 3. `FViewUniformShaderParameters`

它给 shader 提供深度重建、视图矩阵、投影参数等关键信息。

### 4. `FSceneTextureUniformParameters`

它让 shader 能用 UE 原生方式访问场景深度。

### 5. `AddDrawScreenPass`

它是真正把“全屏像素着色器”加进 RDG 的那一步。

### 6. `CalcSceneDepth(PixelPos)`

它是这版深度可视化最核心的一句，完成了：

- 从深度纹理取值
- 从 `DeviceZ` 恢复线性深度

## 你现在可以怎么继续学

如果你已经看懂这篇，下一步最自然的是：

### 1. 先改“灰度映射范围”

比如把：

```hlsl
LinearDepth / 10000.0f
```

改成：

```hlsl
LinearDepth / 3000.0f
```

或者：

```hlsl
LinearDepth / 50000.0f
```

直接观察画面变化。

这是理解深度可视化最直接的方法。

### 2. 下一版做“法线可视化”

因为你已经掌握了：

- 如何在后处理阶段加 pass
- 如何从 shader 读场景数据

接下来最适合的就是继续做法线可视化。

### 3. 再下一步才是 Blur / SSAO

顺序建议还是：

`深度 -> 法线 -> Blur -> SSAO`

## 一句话总结

当前这版灰度图本质上只做了三件事：

1. 在 `Tonemap` 后插入一个全屏 RDG Pass  
2. 在 Pixel Shader 里用 `CalcSceneDepth(PixelPos)` 读线性深度  
3. 把深度压到 `0~1`，再复制到 `RGB` 作为灰度输出

如果你能把这三件事真正讲清楚，这一版代码你就算是学会了。
