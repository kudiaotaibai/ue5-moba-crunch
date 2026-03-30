# RDGStarter 双 Pass Gaussian Blur 讲解

## 目标

这版 `GaussianBlur` 做的不是“显示现成的场景数据”，而是基于当前的 `SceneColor` 重新生成一张新的图像。

对应文件：

- [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)
- [RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf)
- [RDGStarterModule.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp)

## 为什么是双 Pass

二维高斯模糊可以拆成两个一维模糊：

1. 先做横向模糊
2. 再做纵向模糊

这样做的原因是高斯核可分离。

如果半径是 `R`：

- 直接做二维卷积，单像素大约需要 `(2R + 1) * (2R + 1)` 次采样
- 拆成横向 + 纵向两个 pass，单像素只需要 `2 * (2R + 1)` 次采样

这就是双 pass 的核心价值：结果接近二维高斯模糊，但采样成本明显更低。

## 入口是怎么被触发的

调试模式由控制台变量驱动：

- `r.RDGStarter.DebugMode = 5`
- `r.RDGStarter.BlurRadius`
- `r.RDGStarter.BlurSigma`

在 [RDGStarterModule.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterModule.cpp) 里，浮动窗口只是这些 CVar 的一个 UI 壳子。真正读参数和执行渲染的是 [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp)。

## C++ 这一侧的执行链

### 1. 进入后处理回调

在 [RDGStarterSceneViewExtension.cpp](/E:/ue/Crunch/Plugins/RDGStarter/Source/RDGStarter/Private/RDGStarterSceneViewExtension.cpp) 的 `PostProcessPassAfterTonemap_RenderThread()` 里，先拿到当前帧的 `SceneColor`。

这里的 `SceneColor` 是本次后处理阶段的输入纹理，不是原始场景几何数据。

### 2. 读取 BlurRadius / BlurSigma

`GaussianBlur` 分支里会读取：

- `r.RDGStarter.BlurRadius`
- `r.RDGStarter.BlurSigma`

其中：

- `BlurRadius` 决定采样半径
- `BlurSigma` 决定权重衰减速度

### 3. 在 CPU 上生成一维高斯核

`BuildGaussianKernel()` 会把核转换成 `SampleOffsetsWeights` 数组。

每个元素本质上是：

- `x`：当前样本相对中心像素的偏移
- `y`：该样本的高斯权重

例如半径为 `2` 时，会生成这类离散样本：

- `-2`
- `-1`
- `0`
- `1`
- `2`

然后再把所有权重归一化，保证总和接近 `1`。

### 4. 创建中间纹理

`GaussianBlur` 不是直接在输入纹理上读写同一个目标。

这一步会先创建一张 RDG 临时纹理：

- `RDGStarter.GaussianBlurTemp`

用途是：

- 横向 pass 写入中间结果
- 纵向 pass 读取中间结果并输出最终图像

### 5. 第一个 Pass：横向模糊

横向 pass 的关键参数是：

- 输入：`SceneColor`
- 输出：`BlurIntermediate`
- `BlurStep = (1 / Width, 0)`

这意味着 shader 中每次偏移都只沿着 `U` 方向移动，也就是屏幕水平方向。

### 6. 第二个 Pass：纵向模糊

纵向 pass 的关键参数是：

- 输入：`BlurIntermediate`
- 输出：最终 `Output`
- `BlurStep = (0, 1 / Height)`

这意味着 shader 中每次偏移都只沿着 `V` 方向移动，也就是屏幕竖直方向。

## Shader 这一侧做了什么

在 [RDGStarterFullscreen.usf](/E:/ue/Crunch/Plugins/RDGStarter/Shaders/Private/RDGStarterFullscreen.usf) 里，`MainGaussianBlurPS()` 的逻辑很简单：

1. 取当前像素的屏幕 UV
2. 遍历 `SampleOffsetsWeights`
3. 用 `BlurStep * Offset` 计算当前样本 UV
4. 采样输入纹理
5. 乘以当前样本权重
6. 把所有样本累加

关键表达式是：

```hlsl
Color += SampleInputTexture(UV + BlurStep * Offset) * Weight;
```

这里：

- `Offset` 决定“离中心多远”
- `BlurStep` 决定“沿哪个方向移动一个 texel”

所以同一个 shader 可以复用两次：

- 横向 pass 传水平 `BlurStep`
- 纵向 pass 传竖直 `BlurStep`

## Radius 和 Sigma 在当前实现里的意义

### BlurRadius

它决定一侧最多采几个邻居。

当前实现里，每个 pass 的采样数是：

`2 * BlurRadius + 1`

例如：

- `Radius = 1`，采样 `3` 次
- `Radius = 5`，采样 `11` 次
- `Radius = 16`，采样 `33` 次

### BlurSigma

它决定高斯分布的宽度，也就是权重衰减速度。

`Sigma` 越小：

- 中心样本权重越高
- 远处样本衰减越快
- 模糊更“紧”

`Sigma` 越大：

- 远处样本仍然有明显权重
- 模糊更“散”

## 这版代码最值得记住的点

1. `GaussianBlur` 不再是固定 5 tap，而是 CPU 动态生成核，再把数组传给 shader。
2. 双 pass 的关键不是“写两个 shader”，而是“同一个 shader，用不同的 `BlurStep` 跑两次”。
3. RDG 临时纹理的意义是把横向结果保存下来，供纵向 pass 继续读。
4. `BlurRadius` 决定采样范围，`BlurSigma` 决定权重分布，这两个参数不是一回事。

