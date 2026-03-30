# RDGStarter 深度可视化排错总结

## 结果

当前插件已经成功在 UE5.5 工程里跑通：

- `Project Plugin`
- `Scene View Extension`
- `Global Shader`
- `RDG Screen Pass`
- `深度灰度可视化`

表现上，视口会被一张灰度深度图覆盖：

- 近处更暗
- 远处更亮
- 天空和超远处接近白色

这说明整条链路已经真正生效，而不只是“插件被启用”。

## 第一个问题：插件启用了，但效果完全不出现

### 现象

- 插件在 `Plugins` 面板里已经启用
- 模块日志能看到启动
- 画面仍然是原始场景
- `SubscribeToPostProcessingPass()` 没有任何日志

### 根因

`Scene View Extension` 创建时机太早。

最开始是在模块 `StartupModule()` 里直接调用：

```cpp
SceneViewExtension = FSceneViewExtensions::NewExtension<FRDGStarterSceneViewExtension>();
```

但在这个阶段，`GEngine->ViewExtensions` 还不一定准备好。结果就是：

- 扩展对象创建成功了
- 但没有真正进入后续帧的 `ViewFamily.ViewExtensions`
- 因此不会执行 `SubscribeToPostProcessingPass()`
- 更不会进入我们的 RDG 回调

也就是说，这不是“插件没加载”，而是“扩展没真正注册到视图扩展系统里”。

### 解决方法

把扩展创建改成“延后注册”：

- 如果 `GEngine` 和 `GEngine->ViewExtensions` 已经可用，就立即创建
- 否则挂到 `FCoreDelegates::OnPostEngineInit`
- 等引擎初始化完成后再创建扩展

核心思路是：

```cpp
if (GEngine && GEngine->ViewExtensions)
{
    CreateSceneViewExtension();
}
else
{
    PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
        this,
        &FRDGStarterModule::CreateSceneViewExtension);
}
```

修完之后，`SubscribeToPostProcessingPass()` 和真正的后处理回调都能执行。

## 第二个问题：回调执行了，但编辑器直接崩溃

### 现象

日志已经能看到：

- `SubscribeToPostProcessingPass()` 被调用
- `Tonemap` pass 成功注册回调
- `PostProcessPassAfterTonemap_RenderThread()` 真正执行

但随后编辑器直接崩溃，典型报错是：

```text
Shader compilation failures are Fatal.
Failed to create graphics pipeline state
```

### 根因

问题不在 RDG 链路本身，而在“场景深度参数传给 Shader 的方式”。

一开始为了图省事，插件里自己拼了一套场景纹理参数访问方式。虽然 C++ 能编过，但在真正执行到 D3D12 PSO 创建时，Shader 参数布局和引擎期望的用法不完全一致，最终触发了：

- shader 编译失败
- graphics PSO 创建失败
- 编辑器以 fatal 方式崩溃

简单说：

- “回调执行了”不代表 shader 参数绑定一定正确
- RDG pass 能加入图，不代表 GPU 那边的 PSO 一定合法

### 解决方法

改成 UE 原生写法，不再自己拆场景纹理参数，而是直接使用：

- `FSceneTextureUniformParameters`
- `CreateSceneTextureUniformBuffer(...)`
- `SceneTexturesCommon.ush`

像素着色器参数改成：

```cpp
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
```

渲染线程里改成：

```cpp
PassParameters->SceneTextures =
    CreateSceneTextureUniformBuffer(GraphBuilder, View, ESceneTextureSetupMode::SceneDepth);
```

Shader 里则不再手写深度纹理声明，而是直接使用引擎提供的接口：

```hlsl
#include "/Engine/Private/SceneTexturesCommon.ush"

const float LinearDepth = CalcSceneDepth(PixelPos);
```

这一步的核心价值是：

- 参数布局和引擎内置 pass 对齐
- `SceneTexturesStruct` 的定义和采样方式完全走 UE 现成路径
- 避免“C++ 编译通过，但运行期 PSO 非法”的坑

## 这次真正跑通的关键结论

### 1. 插件启用不等于效果会生效

还必须确认：

- 模块是否启动
- Shader 目录是否注册
- `Scene View Extension` 是否真的进入 `ViewFamily.ViewExtensions`
- `SubscribeToPostProcessingPass()` 是否被调用
- RDG 回调是否真正执行

任何一层断掉，最终效果都不会出现。

### 2. 后处理回调能执行，不等于 shader 参数没问题

如果：

- RDG pass 已经加进图
- 但场景纹理 uniform buffer 用法不对

那就可能出现：

- shader 编译失败
- PSO 创建失败
- 编辑器直接 fatal

### 3. 新手阶段最稳的策略是“尽量贴近引擎官方范式”

这次最终稳定版本靠的不是“更聪明的自定义写法”，而是尽量对齐引擎已有模式：

- C++ 侧传 `FSceneTextureUniformParameters`
- HLSL 侧包含 `SceneTexturesCommon.ush`
- 深度读取使用 `CalcSceneDepth(...)`

这会比自己拼 scene texture 参数更稳。

## 现在这个版本还有什么局限

目前这只是“第一版深度可视化”，不是最终效果版：

- 深度映射仍然是简单线性归一化
- 超远处很容易过白
- 没有加调参开关
- 没有做法线重建和深度范围压缩

所以它现在的价值主要是：

- 验证渲染插件链路
- 学会把场景深度从 UE 传进 shader
- 为下一步法线可视化、Blur、SSAO 打基础

## 适合发博客时的标题方向

- `UE5 RDG 渲染插件入门：从纯色全屏 Pass 到深度可视化`
- `UE5 Scene View Extension 踩坑记录：插件启用但效果不生效的原因`
- `UE5 RDG 深度可视化实战：一次 SceneTexture 参数传递导致的崩溃排查`

## 一句话总结

这次最大的收获不是“写出了深度图”，而是搞清楚了两件事：

- `Scene View Extension` 必须在正确时机注册，否则插件启用了也不会生效
- `SceneTexture` 参数最好走 UE 原生 uniform buffer 路线，否则很容易在运行时因为 shader / PSO 问题直接崩溃
