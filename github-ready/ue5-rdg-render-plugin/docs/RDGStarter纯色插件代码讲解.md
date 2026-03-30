# RDGStarter 纯色插件代码讲解

## 1. 这份文档讲什么

这份文档只讲当前已经写出来并且编译通过的最小版本：

- 一个 `Project Plugin`
- 一个 `Scene View Extension`
- 一个 `Global Shader`
- 一个最小 `RDG Pass`
- 最终把画面刷成纯红色

目标不是让你一次把所有 UE 渲染细节都吃透，  
而是让你先真正看懂：

**这套代码现在到底是怎么跑起来的。**

## 2. 先看整体链路

当前这版代码的运行顺序是：

1. UE 读取插件描述文件
2. UE 加载 `RDGStarter` 模块
3. 模块启动时注册 Shader 目录
4. 模块创建 `Scene View Extension`
5. 每帧渲染时，UE 在 `Tonemap` 后调用这个扩展
6. 扩展函数里添加一个全屏 `RDG Pass`
7. 这个 Pass 跑我们自己的像素着色器
8. 像素着色器返回固定红色

所以这版最核心的一句话就是：

**模块负责接入，扩展负责进入每帧渲染，Pass 负责调用 Shader，Shader 负责输出颜色。**

## 3. 相关文件一览

当前最关键的文件是：

- [RDGStarter.uplugin](/E:/ue/Crunch/Plugins/RDGStarter/RDGStarter.uplugin)
- [RDGStarter.Build.cs](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/RDGStarter.Build.cs)
- [RDGStarterModule.h](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Public/RDGStarterModule.h)
- [RDGStarterModule.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp)
- [RDGStarterSceneViewExtension.h](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Public/RDGStarterSceneViewExtension.h)
- [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)
- [RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf)

## 4. 先从插件描述文件开始

文件：
[RDGStarter.uplugin](/E:/ue/Crunch/Plugins/RDGStarter/RDGStarter.uplugin#L12)

这里最重要的是这几项：

- `Name = "RDGStarter"`
- `Type = "Runtime"`
- `LoadingPhase = "PostConfigInit"`

你现在可以先这样理解：

- `Runtime`
  说明这个模块是运行时模块，不是纯编辑器模块

- `PostConfigInit`
  说明它会在比较早的阶段加载

这里的作用不是“做渲染”，而是：

**告诉 UE：这个插件存在，而且里面有一个叫 `RDGStarter` 的运行时模块要加载。**

## 5. `Build.cs` 在做什么

文件：
[RDGStarter.Build.cs](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/RDGStarter.Build.cs)

这个文件主要做的是编译依赖声明。

最关键的是它依赖了：

- `Projects`
- `RenderCore`
- `Renderer`
- `RHI`

你现在不用背每个模块的精确定义，  
先知道这个就够了：

- `Projects`
  让我们能拿到插件路径之类的工程信息

- `RenderCore / Renderer / RHI`
  让我们能写渲染扩展、Shader 和 RDG 相关代码

所以这个文件的本质是：

**让这套渲染插件代码具备编译资格。**

## 6. `RDGStarterModule.h` 是什么

文件：
[RDGStarterModule.h](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Public/RDGStarterModule.h)

这里有两个关键点：

- 定义了日志分类 `LogRDGStarter`
- 声明了模块类 `FRDGStarterModule`

模块类里最关键的成员是：

- `SceneViewExtension`

它是一个 `TSharedPtr`，用来持有我们的渲染扩展对象。

为什么要持有？

因为 `Scene View Extension` 不是临时调用一次就结束的东西，  
它需要在模块生命周期内一直活着，UE 才能每帧回调它。

## 7. `RDGStarterModule.cpp` 是真正的启动入口

文件：
[RDGStarterModule.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp)

这个文件你可以先重点看三个地方。

### 7.1 日志分类

在 [9 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L9)：

```cpp
DEFINE_LOG_CATEGORY(LogRDGStarter);
```

这只是定义插件自己的日志类别，方便后面输出调试信息。

### 7.2 `StartupModule()`

在 [11 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L11) 开始。

这是真正的运行时入口。

你可以把它理解成：

**“插件模块被 UE 加载后，第一个会执行的重要函数。”**

### 7.3 先找插件目录

在 [13-18 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L13)：

- 用 `IPluginManager` 查找插件
- 如果没找到就报错并返回

这一步的目的很直接：

**后面注册 Shader 目录时，需要先知道插件本体在磁盘上的位置。**

### 7.4 注册 Shader 目录映射

在 [20-22 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L20)：

```cpp
const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
AddShaderSourceDirectoryMapping(TEXT("/RDGStarter"), PluginShaderDir);
```

这是这个文件里最关键的一步之一。

为什么要这样做？

因为后面 `IMPLEMENT_GLOBAL_SHADER` 用的是虚拟路径：

`/RDGStarter/Private/RDGStarterFullscreen.usf`

如果你不先在这里告诉 UE：

- `/RDGStarter` 对应磁盘上的哪个目录

那引擎后面是找不到 `.usf` 文件的。

所以这一步的意义是：

**把插件磁盘目录映射成 Shader 可识别的虚拟路径。**

### 7.5 创建 `Scene View Extension`

在 [24 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L24)：

```cpp
SceneViewExtension = FSceneViewExtensions::NewExtension<FRDGStarterSceneViewExtension>();
```

这是整条链里的第二个关键动作。

它的意思是：

**创建并注册一个渲染扩展对象，让 UE 后续每帧渲染时都能调用它。**

注意这里不是你自己手动每帧调用它，  
而是你把它注册给了 UE，之后由 UE 驱动执行。

### 7.6 `ShutdownModule()`

在 [28-31 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L28)：

这里只做了一件事：

- `SceneViewExtension.Reset();`

意思是模块关闭时释放扩展对象。

## 8. `RDGStarterSceneViewExtension.h` 在声明什么

文件：
[RDGStarterSceneViewExtension.h](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Public/RDGStarterSceneViewExtension.h)

这里最关键的是这个类：

- `FRDGStarterSceneViewExtension : public FSceneViewExtensionBase`

这说明它是一个标准的 Scene View Extension。

你现在重点看两个成员函数：

- `SubscribeToPostProcessingPass(...)`
- `PostProcessPassAfterTonemap_RenderThread(...)`

这两个函数分别负责：

- 告诉 UE：我要订阅哪个后处理阶段
- 当 UE 真走到那个阶段时，我实际要干什么

## 9. `RDGStarterSceneViewExtension.cpp` 是核心中的核心

文件：
[RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)

这个文件建议你按下面顺序看。

## 10. 先看文件顶部的 Shader 类

在 [10-27 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L10)

这里声明了一个最小像素着色器类：

- `FRDGStarterColorPS`

它继承自：

- `FGlobalShader`

这意味着：

**它不是材质系统里的 Shader，而是由 C++ 直接控制的全局 Shader。**

### 10.1 `FParameters`

在 [16-19 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L16)：

```cpp
BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER(FVector4f, FillColor)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()
```

这个参数结构体只有两类内容：

- `FillColor`
  这是我们要传给 Shader 的颜色

- `RENDER_TARGET_BINDING_SLOTS`
  这是输出目标绑定

也就是说，当前这个 Shader 非常纯粹：

**输入一个颜色，输出到目标纹理。**

### 10.2 `ShouldCompilePermutation`

在 [21-24 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L21)

这里只是限制：

- 至少支持 `SM5`

你可以先简单理解为：

**只在现代桌面级 Shader Model 上编译这段 Shader。**

### 10.3 `IMPLEMENT_GLOBAL_SHADER`

在 [27 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L27)：

```cpp
IMPLEMENT_GLOBAL_SHADER(FRDGStarterColorPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainPS", SF_Pixel);
```

这句非常关键。

它做了三件事：

1. 把 C++ 里的 `FRDGStarterColorPS` 和 `.usf` 文件对应起来
2. 指定入口函数名是 `MainPS`
3. 指定它是一个 Pixel Shader

也就是说：

**C++ 这边的 Shader 类，最终绑定到 `RDGStarterFullscreen.usf` 里的 `MainPS`。**

## 11. 构造函数没做复杂事情

在 [29-32 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L29)

这里只是正常把 `FAutoRegister` 传给父类。

小白阶段你只要知道：

**Scene View Extension 必须通过这种方式被构造和注册。**

## 12. `SubscribeToPostProcessingPass()` 干了什么

在 [34-45 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L34)

这是当前文件里第一个真正决定“在哪里插入效果”的函数。

### 12.1 它在判断什么

```cpp
if (Pass == EPostProcessingPass::Tonemap && bIsPassEnabled)
```

意思是：

只有当 UE 走到 `Tonemap` 这个后处理阶段时，  
我们才插入自己的回调。

### 12.2 它插入了什么

在 [42-43 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L42)：

```cpp
FAfterPassCallbackDelegate::CreateRaw(this, &FRDGStarterSceneViewExtension::PostProcessPassAfterTonemap_RenderThread)
```

意思是：

**当 UE 渲染到 `Tonemap` 后时，调用 `PostProcessPassAfterTonemap_RenderThread()`。**

所以这一步你可以直接理解成：

**我们把自己的渲染逻辑挂到了后处理链的 Tonemap 节点上。**

## 13. `IsActiveThisFrame_Internal()` 为什么现在直接返回 true

在 [47-50 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L47)

现在它直接：

- `return true;`

意思是：

**当前每一帧都启用这个扩展。**

以后你可以在这里加：

- 开关控制
- 只在特定视图启用
- 只在游戏视图启用

但最小版本里，这样最直观。

## 14. 真正的效果函数：`PostProcessPassAfterTonemap_RenderThread()`

在 [52-79 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L52)

这是当前最核心的函数。

你可以把它拆成 5 步看。

### 14.1 第一步：拿到 SceneColor

在 [57-58 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L57)

```cpp
const FScreenPassTexture SceneColor =
	FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor), Inputs.OverrideOutput);
```

意思是：

- 从当前后处理输入里拿到 `SceneColor`
- 把它变成一个可用的 `FScreenPassTexture`

你现在不用深究 slice 细节，  
先理解成：

**这里拿到了当前这一步后处理链上的画面纹理。**

### 14.2 第二步：把它包装成输出目标

在 [60-61 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L60)

```cpp
FScreenPassRenderTarget Output(SceneColor, ERenderTargetLoadAction::ENoAction);
const FScreenPassTextureViewport OutputViewport(Output);
```

这一步的作用是：

- 告诉引擎，我们要往这个目标里画
- 同时拿到对应的 Viewport 信息

这里的 `ENoAction` 可以先粗略理解成：

**这个 Pass 不需要额外清屏，而是直接在这个输出目标上写。**

### 14.3 第三步：准备 Shader 参数

在 [63-65 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L63)

```cpp
PassParameters->FillColor = FVector4f(1.0f, 0.05f, 0.05f, 1.0f);
PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
```

这一步非常重要。

它说明了 CPU 侧在做两件事：

- 给 Shader 传颜色参数
- 告诉这个 Pass 输出写到哪里

现在传的是一个偏红色：

- `R = 1.0`
- `G = 0.05`
- `B = 0.05`
- `A = 1.0`

所以你后面看到的结果应该是整屏偏红。

### 14.4 第四步：拿到 Shader 对象

在 [67-68 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L67)

```cpp
FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.FeatureLevel);
TShaderMapRef<FRDGStarterColorPS> PixelShader(ShaderMap);
```

意思是：

- 根据当前视图的 FeatureLevel 找到 ShaderMap
- 从里面取出我们前面声明的那个 `FRDGStarterColorPS`

也就是说：

**这里拿到的是 C++ 侧对应的那个 Global Shader 实例。**

### 14.5 第五步：真正添加一个全屏 Pass

在 [70-77 行](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L70)

```cpp
AddDrawScreenPass(...)
```

这是当前代码里最重要的一句之一。

它的作用就是：

**往 RDG 里加一个全屏屏幕绘制 Pass。**

这里传进去的关键信息包括：

- `GraphBuilder`
  当前 RDG 图

- `RDG_EVENT_NAME("RDGStarter.SolidColor")`
  这个 Pass 的调试名字

- `FScreenPassViewInfo(View)`
  当前 View 的视图信息

- `OutputViewport`
  输出区域

- `PixelShader`
  这个 Pass 要跑的像素着色器

- `PassParameters`
  Shader 参数和输出绑定

所以你可以把这一步理解成：

**把“整屏跑一遍我们的红色像素着色器”这件事，正式交给 RDG。**

## 15. `.usf` 文件到底做了什么

文件：
[RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf#L3)

这个文件现在只有几行。

关键代码在 [3-6 行](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf#L3)：

```hlsl
float4 MainPS(float4 SvPosition : SV_POSITION) : SV_Target0
{
	return FillColor;
}
```

这段代码你可以这样理解：

- `MainPS`
  就是像素着色器入口函数

- `SV_Target0`
  表示输出到第一个颜色目标

- `return FillColor;`
  表示当前像素直接输出 CPU 传进来的颜色

也就是说，这个 Shader 完全没有做复杂计算。

它只是：

**把 C++ 给的颜色原样写到屏幕像素上。**

这也是为什么它适合做第一个最小效果。

## 16. 当前这版代码每个文件各自的职责

再总结一次。

### 16.1 `.uplugin`

职责：

- 让 UE 识别插件和模块

### 16.2 `Build.cs`

职责：

- 提供渲染相关编译依赖

### 16.3 `RDGStarterModule.cpp`

职责：

- 模块启动
- 注册 Shader 目录
- 创建并持有 `Scene View Extension`

### 16.4 `RDGStarterSceneViewExtension.cpp`

职责：

- 挂到 `Tonemap` 后处理阶段
- 在每帧渲染时添加一个全屏 RDG Pass
- 把颜色参数传给 Shader

### 16.5 `RDGStarterFullscreen.usf`

职责：

- 真正在 GPU 上输出固定颜色

## 17. 这版代码真正验证了什么

只要你在编辑器里能看到整屏偏红，  
就说明这条链大概率已经成立：

1. 插件被 UE 识别
2. 模块正确启动
3. Shader 目录映射成功
4. Scene View Extension 正常注册
5. 后处理订阅成功
6. RDG Pass 确实被加进每帧渲染
7. Global Shader 成功编译并执行

这就是这版最小实现的价值。

## 18. 为什么这版代码还很“简陋”

这是故意的。

因为我们现在要验证的是：

- 工程链路
- 渲染接入
- Shader 调用

而不是：

- 深度读取
- 法线读取
- Blur
- SSAO

如果一上来就堆复杂效果，你会很难判断问题到底出在哪。

现在这版的价值就在于：

**它把变量压到最少。**

## 19. 你现在看代码时最应该抓住的 3 个点

### 19.1 第一个点：模块启动时做了什么

重点看：
[RDGStarterModule.cpp:11](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp#L11)

记住：

- 模块负责注册 Shader 路径
- 模块负责创建扩展对象

### 19.2 第二个点：扩展对象把逻辑挂到哪里

重点看：
[RDGStarterSceneViewExtension.cpp:34](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L34)

记住：

- 当前挂的是 `Tonemap`
- 是通过 `SubscribeToPostProcessingPass` 挂进去的

### 19.3 第三个点：真正画红色的是谁

重点看：
[RDGStarterSceneViewExtension.cpp:63](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp#L63)
和
[RDGStarterFullscreen.usf:3](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf#L3)

记住：

- C++ 侧传 `FillColor`
- `.usf` 侧直接 `return FillColor`

## 20. 这份代码讲到这里，你应该已经能回答的问题

如果你现在能回答下面这些，说明你真的开始看懂了：

- 为什么模块里要注册 Shader 路径？
- 为什么模块要持有 `SceneViewExtension`？
- 为什么渲染逻辑不是写在 `Module.cpp` 里，而是写在 `SceneViewExtension.cpp` 里？
- 为什么当前这版是挂在 `Tonemap` 后？
- 为什么 `.usf` 里只有一句 `return FillColor` 也能形成一个完整效果？

## 21. 最后一句话总结

当前这版代码的本质就是：

**模块启动时把插件 Shader 和渲染扩展挂进 UE，然后在每帧 `Tonemap` 后加一个全屏 RDG Pass，这个 Pass 调用一个最小 Global Shader，把 CPU 传进来的固定红色直接输出到屏幕上。**
