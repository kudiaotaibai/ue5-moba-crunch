# RDGStarter 法线可视化代码讲解

这篇只讲当前这版最小法线可视化实现。

目标很简单：

- 不再显示深度灰度图
- 改为显示场景的世界空间法线
- 让不同朝向的表面直接显示成不同颜色

## 一、这版效果到底是什么

你现在看到的不是物体原本的材质颜色。

现在屏幕上的颜色，表示的是“这个像素表面的朝向”。

比如：

- 法线朝 `+X`，红色分量会更高
- 法线朝 `+Y`，绿色分量会更高
- 法线朝 `+Z`，蓝色分量会更高

所以这类图通常会呈现出彩色，而不是灰度。

它的用途主要是：

- 检查 GBuffer 里的法线是否正确
- 观察模型表面朝向变化
- 为后面的 SSAO、边缘检测、屏幕空间特效打基础

## 二、这版代码整体链路

和上一版深度可视化相比，链路没有变，只是“读什么数据”变了。

完整链路还是：

1. 插件模块启动
2. `Scene View Extension` 挂进 UE 的后处理阶段
3. 每帧进入 `PostProcessPassAfterTonemap_RenderThread`
4. C++ 往 RDG 里加一个全屏 pass
5. 这个 pass 调用 `RDGStarterFullscreen.usf` 里的 `MainPS`
6. `MainPS` 从 GBuffer 里读取法线并输出颜色

## 三、C++ 里改了什么

文件：
[RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)

### 1. Shader 类名变了

```cpp
class FRDGStarterNormalPS : public FGlobalShader
```

这里的 `PS` 是 `Pixel Shader`。

这表示：

- 这是一个像素着色器
- 它会在全屏 pass 里对屏幕上的每个像素执行一次

### 2. 参数结构里依然有 `View`

```cpp
SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
```

这句的意思是：

- 把当前视图的一些统一参数传给 shader
- 比如视图尺寸、投影参数、Buffer 相关信息

虽然这版主要读取法线，但 UE 的很多 shader 辅助函数仍然依赖 `View`。

### 3. 参数结构里最重要的是 `SceneTextures`

```cpp
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
```

这句可以先直接理解成：

- 把一组“场景纹理”打包传给 shader
- shader 里就能用 `SceneTexturesStruct.xxxTexture` 去访问这些纹理

这里的场景纹理包括：

- 深度
- GBufferA / GBufferB / GBufferC ...

而法线就存放在 GBuffer 解码结果里。

### 4. 这次传入的不是纯深度，而是“深度 + GBuffer”

```cpp
PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
    GraphBuilder,
    View,
    ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
```

这是这次最关键的变化。

上一版深度图只需要：

```cpp
ESceneTextureSetupMode::SceneDepth
```

这一版法线图需要：

```cpp
ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers
```

原因是：

- 法线数据主要来自 `GBuffers`
- UE 的 GBuffer 解码辅助函数里，可能顺手也会访问深度

所以这版最稳妥的做法，就是把两者一起传进去。

### 5. `AddDrawScreenPass` 本质没变

```cpp
AddDrawScreenPass(
    GraphBuilder,
    RDG_EVENT_NAME("RDGStarter.NormalVisualization"),
    FScreenPassViewInfo(View),
    OutputViewport,
    OutputViewport,
    PixelShader,
    PassParameters);
```

你可以把它理解成：

- 往 RDG 里注册一个“全屏绘制任务”
- 这个任务使用 `PixelShader`
- shader 参数就是 `PassParameters`

所以真正的变化不在 `AddDrawScreenPass` 本身，而在“shader 参数里现在能访问到 GBuffer 法线”。

## 四、`.usf` 里改了什么

文件：
[RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf)

### 1. 这次引入的是 `DeferredShadingCommon.ush`

```hlsl
#include "/Engine/Private/DeferredShadingCommon.ush"
```

这一步很重要。

因为这个头文件里已经有 UE 提供的很多现成函数，比如：

- `GetGBufferData(UV)`
- `GetScreenSpaceData(UV)`

我们这次就是直接用它来读法线，不自己手搓 GBuffer 解码。

对你现在这个阶段，这种做法更合适。

### 2. 先拿到当前像素的屏幕 UV

```hlsl
const float2 UV = UVAndScreenPos.xy;
```

这里的 `UVAndScreenPos` 是屏幕 pass 顶点阶段传下来的输入。

你现在可以先简单理解成：

- `UV` 表示“当前像素在屏幕纹理上的坐标”
- 范围通常是 `0 ~ 1`

### 3. 用 UE 提供的函数读取 GBuffer

```hlsl
const FGBufferData GBufferData = GetGBufferData(UV);
```

这句是整版 shader 的核心。

它的意思是：

- 根据当前像素的 `UV`
- 去场景的 GBuffer 里读出这个像素对应的材质/法线等信息
- 然后把结果打包成一个 `FGBufferData`

你现在最需要记住的是：

- `FGBufferData` 是一个“解码后的 GBuffer 结果”
- 它里面有 `WorldNormal`

### 4. 取出世界空间法线

```hlsl
const float3 WorldNormal = normalize(GBufferData.WorldNormal);
```

这里的 `WorldNormal` 是三维向量：

```text
(x, y, z)
```

它表示“表面朝向哪个方向”。

例如：

- `(0, 0, 1)` 表示朝上
- `(1, 0, 0)` 表示朝右
- `(-1, 0, 0)` 表示朝左

这里再做一次 `normalize(...)`，是为了确保它是单位向量。

### 5. 为什么要 `* 0.5 + 0.5`

```hlsl
const float3 NormalColor = WorldNormal * 0.5f + 0.5f;
```

这是这版最重要的图形学小知识之一。

法线分量范围通常是：

```text
-1 到 1
```

但屏幕颜色范围通常是：

```text
0 到 1
```

所以不能直接把法线当颜色输出。

要先做一个范围映射：

```text
-1 -> 0
 0 -> 0.5
 1 -> 1
```

这个映射公式就是：

```text
NewValue = OldValue * 0.5 + 0.5
```

所以：

- `(-1, -1, -1)` 会变成黑色附近
- `(0, 0, 1)` 会变成偏蓝色
- `(1, 0, 0)` 会变成偏红色

### 6. 最后输出到屏幕

```hlsl
return float4(NormalColor, 1.0f);
```

意思就是：

- RGB 用我们刚算出来的法线颜色
- A 直接给 `1.0`

于是这个像素就被画到屏幕上了。

## 五、为什么这版比深度图更进一步

深度图主要回答的是：

- 这个像素离相机有多远

法线图回答的是：

- 这个像素表面朝向哪里

所以法线图更接近很多屏幕空间特效真正需要的输入。

后面这些效果都会经常用到法线：

- SSAO
- 边缘检测
- 屏幕空间轮廓
- 一些 NPR 后处理

## 六、你现在最该记住的 3 件事

### 1. 这次不是换 RDG 链路，而是换“输入数据”

RDG 的链路没变：

- 还是 `Scene View Extension`
- 还是 `AddDrawScreenPass`
- 还是 `MainPS`

真正变的是：

- 从读深度
- 变成了读 GBuffer 法线

### 2. 法线可视化的关键是 `GetGBufferData(UV)`

你现在不用自己解码 GBuffer。

先学会：

- C++ 传 `SceneDepth | GBuffers`
- `.usf` 里调用 `GetGBufferData(UV)`

这就足够了。

### 3. 法线能显示成颜色，是因为做了范围映射

法线本来是：

```text
[-1, 1]
```

颜色要的是：

```text
[0, 1]
```

所以才有这句：

```hlsl
WorldNormal * 0.5 + 0.5
```

## 七、下一步该学什么

你现在最适合的下一步，不是马上上 SSAO，而是继续做下面两个之一：

1. 给法线可视化加一个开关
2. 写一版“深度 + 法线”双调试视图

如果继续往图形学本身走，最自然的下一步是：

- 搞清楚 `GBuffer` 到底是什么
- 搞清楚为什么延迟渲染里会把法线、粗糙度、材质信息拆开存

那样你后面看 SSAO、SSR、Deferred Lighting 都会顺很多。
