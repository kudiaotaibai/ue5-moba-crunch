# RDGStarter 最小调试视图切换器讲解

这篇只解释当前这版最小调试视图切换器，不展开讲更多渲染理论。

当前支持的模式：

- `0`：关闭
- `1`：深度可视化
- `2`：法线可视化
- `3`：原始 `SceneColor`

相关文件：

- [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)
- [RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf)

## 1. 整体思路

这版不是做多个插件，也不是做多个独立后处理材质。

它的结构很简单：

1. 用一个控制台变量表示当前调试模式
2. `SceneViewExtension` 每帧读取这个模式
3. 根据模式决定：
   - 本帧是否生效
   - 走深度 shader
   - 走法线 shader
   - 还是直接透传原图

也就是说，切换逻辑在 C++，真正的可视化逻辑在 shader。

## 2. 模式值从哪里来

在 [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp) 里定义了一个控制台变量：

```cpp
static TAutoConsoleVariable<int32> CVarRDGStarterDebugMode(
    TEXT("r.RDGStarter.DebugMode"),
    2,
    ...
);
```

它的作用就是：

- 把当前调试模式挂到 UE 控制台系统里
- 让你在编辑器里直接输入命令切换

例如：

```text
r.RDGStarter.DebugMode 0
r.RDGStarter.DebugMode 1
r.RDGStarter.DebugMode 2
r.RDGStarter.DebugMode 3
```

这里默认值是 `2`，所以插件启动后默认显示法线可视化。

## 3. “关闭”是怎么实现的

关闭不是“shader 继续执行，只是输出原图”。

关闭的实现是在：

```cpp
bool FRDGStarterSceneViewExtension::IsActiveThisFrame_Internal(...) const
{
    return CVarRDGStarterDebugMode.GetValueOnAnyThread() != 0;
}
```

这句的含义是：

- 如果模式值是 `0`
- 这一帧 `SceneViewExtension` 直接失活

结果就是：

- UE 不会把这个扩展当成本帧活跃扩展
- 后处理回调不会参与
- RDG pass 根本不会被添加

所以 `0` 的语义非常清晰：

**不是“显示原图”，而是“整个调试系统本帧不工作”。**

## 4. 为什么 `3` 不等于 `0`

`3` 表示“显示原始 `SceneColor`”，但它不是彻底关闭。

在 [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp) 里，你会看到：

```cpp
const uint32 VisualizationMode = static_cast<uint32>(FMath::Clamp(..., 0, 3));
if (VisualizationMode == 3)
{
    return SceneColor;
}
```

这表示：

- 扩展本帧仍然是活跃的
- 也已经进入了后处理回调
- 只是我们选择不再添加新的可视化 pass
- 而是把当前输入的 `SceneColor` 直接返回

所以：

- `0` = 不参与
- `3` = 参与了，但选择“输出原图”

这两个模式在实现语义上是不同的。

## 5. 为什么切换逻辑放在 C++ 而不是 shader

这次我们最后采用的是：

- C++ 决定绑定哪个 shader
- shader 只负责各自单一的显示逻辑

原因很直接：

- 代码更稳定
- 更容易排错
- 更符合你当前学习阶段

也就是说，我们没有把“深度/法线/原图/关闭”全部塞进一个 shader 参数分支里，而是做成：

- 深度模式绑定 `FRDGStarterDepthPS`
- 法线模式绑定 `FRDGStarterNormalPS`
- 原图模式直接返回 `SceneColor`

这样一来，每个分支都更独立。

## 6. 深度和法线各自怎么走

在 `PostProcessPassAfterTonemap_RenderThread(...)` 里，流程大致是：

1. 先拿到当前后处理输入 `SceneColor`
2. 如果模式是 `3`，直接返回
3. 否则创建输出目标 `Output`
4. 如果模式是 `1`，绑定深度 shader
5. 否则绑定法线 shader
6. 调用 `AddDrawScreenPass(...)`

可以简化理解为：

```text
输入 SceneColor
    -> 模式 3：直接返回
    -> 模式 1：加一个深度全屏 pass
    -> 模式 2：加一个法线全屏 pass
```

## 7. 深度 shader 干了什么

在 [RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf) 里：

```hlsl
float4 MainDepthPS(...) : SV_Target0
{
    const float2 UV = UVAndScreenPos.xy;
    const float LinearDepth = CalcSceneDepth(UV);
    const float DepthGray = saturate(LinearDepth / 10000.0f);
    return float4(DepthGray.xxx, 1.0f);
}
```

它只做一件事：

- 读取深度
- 把深度映射成灰度

所以这是“距离信息调试图”。

## 8. 法线 shader 干了什么

同一个文件里：

```hlsl
float4 MainNormalPS(...) : SV_Target0
{
    const float2 UV = UVAndScreenPos.xy;
    const FGBufferData GBufferData = GetGBufferData(UV);
    const float3 WorldNormal = normalize(GBufferData.WorldNormal);
    const float3 NormalColor = WorldNormal * 0.5f + 0.5f;
    return float4(NormalColor, 1.0f);
}
```

它做的是：

- 从 GBuffer 里读取当前像素的世界空间法线
- 把 `[-1, 1]` 映射到 `[0, 1]`
- 作为 RGB 输出

所以这是“表面朝向调试图”。

## 9. 为什么深度和法线用两个 shader 更合适

从工程角度，这种拆法有几个优点：

- 每个 shader 的职责单一
- 编译错误更容易定位
- 后面继续加模式时，扩展方式更清楚

例如以后你想加：

- `4 = AO`
- `5 = Roughness`
- `6 = Metallic`

最自然的方式就是继续在 C++ 里分支，然后绑定新的 shader 入口或新的 shader 类。

## 10. 当前这版最重要的设计点

这版最值得记住的不是语法，而是这三个设计点：

### 10.1 关闭和原图不是同一个概念

- `0` 是扩展失活
- `3` 是扩展活跃但直接透传

### 10.2 切换逻辑在 C++

C++ 决定当前模式对应哪条渲染路径。

### 10.3 每个 shader 只做一件事

- 深度 shader 只管深度
- 法线 shader 只管法线

这会让整个系统更稳定、更容易继续演进。

## 11. 你现在可以怎么用

在编辑器控制台中输入：

```text
r.RDGStarter.DebugMode 0
r.RDGStarter.DebugMode 1
r.RDGStarter.DebugMode 2
r.RDGStarter.DebugMode 3
```

建议这样验证：

1. 先用 `3` 看原始画面
2. 切到 `1` 看深度图
3. 切到 `2` 看法线图
4. 最后切到 `0`，确认插件完全不参与

这样你会非常直观地理解“透传”和“关闭”的区别。

## 12. 下一步适合做什么

如果继续沿着“调试视图切换器”走，下一步最自然的是：

- 加一个 `4 = Roughness`
- 或者做一个简单 UI，而不是只靠控制台命令切换

如果继续沿着“学习 RDG”走，下一步最适合的是：

- 补一篇 `AddDrawScreenPass` 是如何把 shader 真正画到屏幕上的
