# UE5 RDG 渲染插件项目说明

## 1. 这个项目具体是什么

这个项目不是“从零写一个渲染器”，也不是“做一个普通后处理材质”。

它的定位是：

- 在 **UE5.5 项目内**做一个独立的 **C++ 渲染插件**
- 插件通过 **Scene View Extension + Global Shader + RDG Pass**
  把我们自己的渲染逻辑插入到 UE 的渲染流程里
- 第一阶段目标是做出一个 **可开关、可调参数、可调试可视化** 的屏幕空间效果

更直接地说，这个项目是在学：

- UE 里渲染代码怎么接入
- HLSL Shader 怎么和 C++ 参数结构体对应
- RDG 怎么创建资源、组织 Pass、管理依赖
- 一个图形学效果怎么从“公式”落到“能在屏幕上跑”

这个插件本身就是一个“图形学实验平台”。  
后面你可以往里面逐步加效果：

- Depth / Normal Debug View
- Box Blur / Gaussian Blur / Bilateral Blur
- Sobel Edge
- SSAO
- 简单的屏幕空间雾
- 屏幕空间描边

其中最适合写进简历的第一版目标，是：

**基于 UE5 RDG 的屏幕空间效果插件，首个完整效果选择 SSAO，并配套 Debug 可视化与模糊降噪。**

## 2. 为什么选这个方向

### 2.1 和你现在的背景衔接最好

你现在已经有：

- UE5 C++
- Gameplay Framework
- GAS
- UI
- 联机和服务端

但简历上还缺一个更“图形学/引擎向”的技术点。  
RDG 渲染插件正好补这个空位，而且不用换技术栈。

### 2.2 学到的是 UE 当前主流渲染开发方式

Epic 现在的官方文档里，RDG 已经是高级渲染代码的标准写法之一。  
如果你以后想接触：

- Renderer 模块
- 自定义后处理
- 计算着色器
- 屏幕空间效果
- 虚幻引擎插件级渲染扩展

这个项目是正确入口。

### 2.3 比纯 Material 更像“图形学项目”

如果只做 Post Process Material，你当然也能做出效果，但学习重点会更偏：

- 材质节点搭建
- 引擎现成输入输出的使用

而不是：

- 渲染线程接入
- Pass 组织
- Shader 参数绑定
- RDG 资源生命周期
- 插件级渲染扩展

所以从“简历价值”和“边做边学”的角度，**Global Shader + RDG 插件**比纯材质方案更合适。

## 3. 技术选型

这次项目建议固定用下面这套，不要一开始分叉。

### 3.1 引擎版本

- **Unreal Engine 5.5**

原因：

- 你的现有项目就是 UE5.5
- 不需要先做版本迁移
- 后面查官方文档时，5.5 到 5.7 的 RDG 思路基本一致

### 3.2 工程载体

- **Project Plugin**

原因：

- 独立于主游戏逻辑
- 更适合管理 Shader、模块和渲染接入代码
- 后面简历里可以明确写成“自研 UE5 渲染插件”

### 3.3 核心语言与文件形式

- **C++**
- **HLSL / USF Shader**

原因：

- C++ 负责插件模块、Pass 注册、参数组织、生命周期管理
- USF 负责真正的 GPU 计算与像素处理

### 3.4 渲染接入方式

- **Scene View Extension**

原因：

- 它是 UE 官方提供的视图扩展入口
- 适合在不改引擎源码的情况下接入自定义渲染逻辑
- 比直接魔改 Renderer 模块更稳，也更适合个人项目

### 3.5 Pass 组织方式

- **RDG（FRDGBuilder + RDG Pass）**

原因：

- RDG 会负责资源依赖、Pass 执行顺序、临时资源生命周期
- 这是 UE 当前高级渲染代码的主流写法
- 你以后学 Compute Pass、Async Compute、更多后处理时都能复用

### 3.6 Shader 类型

- **Global Shader**

原因：

- 适合做插件内的独立渲染效果
- 比 Material Shader 更贴近底层渲染开发
- 配合全屏 Pass 非常适合做屏幕空间效果

### 3.7 首个效果的选择

- **SSAO + Blur + Debug View**

原因：

- 有明确图形学含量
- 结果直观，容易展示
- 技术点完整，能覆盖采样、深度重建、噪声、屏幕空间、后处理链路
- 面试时容易展开

如果你觉得第一步直接 SSAO 偏陡，可以先临时做两个热身效果：

- Depth/Normal 可视化
- Gaussian Blur

但正式项目标题仍然建议收敛到 **SSAO 插件**。

## 4. 这个项目的应用与目的

### 4.1 学习目的

这个项目主要用来建立你对以下内容的“工程级理解”：

- UE 的渲染扩展入口在哪里
- CPU 怎么准备渲染参数，GPU 怎么消费这些参数
- 一个渲染效果为什么要拆成多个 Pass
- 为什么要用中间纹理，为什么要做 Blur
- 图像空间效果和世界空间效果的区别

### 4.2 简历目的

做完以后，你的简历里可以多一条明显区别于普通 UE 客户端同学的项目经历：

- 会 GAS 和联机的人不少
- 会在 UE 里自己写 RDG Pass、Global Shader、屏幕空间效果的人少得多

### 4.3 面试目的

它可以帮你覆盖下面这些面试话题：

- 渲染管线基础
- Render Thread / GPU Pass
- Shader 参数结构体
- 深度、法线、屏幕空间重建
- 后处理链路
- 图形学效果的性能和画质权衡

## 5. 你现在需要的前置知识

这里不要求你全懂，但至少知道“是什么”。

### 5.1 数学基础

最低要求：

- 向量、点积、叉积
- 矩阵乘法
- 坐标空间变换
- 归一化
- 插值

做 SSAO 时还会用到：

- 切线空间/视空间
- 半球采样
- 随机旋转
- 深度重建

### 5.2 图形学基础

至少先建立这些概念：

- GPU 渲染管线在干什么
- Vertex Shader 和 Pixel Shader 的职责
- Render Target 是什么
- Texture Sampling 是什么
- Fullscreen Pass 是什么
- Depth Buffer / Normal Buffer 是什么

### 5.3 Shader 基础

你需要能看懂并写出最基础的 HLSL：

- 输入输出结构体
- 纹理采样
- UV 计算
- 常量参数
- for 循环与简单分支

不需要一开始就会复杂优化，但要能独立看懂一个简单的全屏像素着色器。

### 5.4 UE 渲染基础概念

需要先认识这些关键词：

- Render Thread
- RHI
- RDG
- Shader Parameter Struct
- Global Shader
- Screen Pass
- Scene View Extension

你现在不用把源码全读完，但这些词以后会反复出现。

### 5.5 UE 插件与模块基础

你需要知道：

- `.uplugin` 是干什么的
- 模块怎么声明
- Shader 文件一般放在哪里
- 插件为什么要在特定 LoadingPhase 初始化

这个属于 UE 工程组织知识，不难，但必须补。

### 5.6 C++ 基础要求

这部分你已经有基础了，但在渲染项目里要特别注意：

- 结构体定义
- 枚举
- 模板和宏不要害怕
- 生命周期和线程边界要敏感

尤其是 RDG 的 Lambda，不要把“只在当前栈帧有效”的对象错误捕获进去。

## 6. 这类项目里最重要的几个认知

### 6.1 RDG 不是效果本身，它是“组织效果的方式”

不要把 RDG 理解成某种 SSAO 算法。  
RDG 只是帮你管理：

- 资源
- 依赖
- Pass
- 执行顺序

真正的效果还是靠 Shader 和你的算法。

### 6.2 插件是“载体”，SSAO 才是“图形学内容”

插件解决的是工程接入问题。  
SSAO、Blur、Edge 这些才是你真正要学的图形学效果。

### 6.3 先跑通，再追求正确和漂亮

这类项目最怕一开始就想做：

- 物理上更正确
- 性能更极致
- 结构更完美

正确顺序应该是：

- 先能接入
- 先能出图
- 再做参数化
- 再做优化

## 7. 你现在不用急着啃的东西

下面这些先不要一上来就深挖：

- Vulkan / DX12 底层 API 细节
- 引擎 Renderer 全模块源码
- Lumen / Nanite 实现
- 复杂异步计算调度
- 时域降噪

第一阶段目标只是：

**在 UE5 里独立做出一个基于 RDG 的可运行屏幕空间效果插件。**

## 8. 官方资料

- 渲染依赖图（RDG）  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/render-dependency-graph-in-unreal-engine

- 新建全局着色器并作为插件  
  https://dev.epicgames.com/documentation/zh-cn/unreal-engine/creating-a-new-global-shader-as-a-plugin-in-unreal-engine

- FSceneViewExtensionBase API  
  https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewExtensionBase

- DrawScreenPass API  
  https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/DrawScreenPass

## 9. 现阶段结论

这个项目最合适的定义是：

**在 UE5.5 中开发一个基于 Scene View Extension、Global Shader 和 RDG 的渲染插件，首个完整效果选用 SSAO，并通过 Debug 可视化与 Blur 形成一个可展示、可讲述、可继续扩展的图形学项目。**

这个方向的优势不是“最快做出炫酷画面”，而是：

- 和你现有背景衔接自然
- 图形学含量足够
- 简历价值高
- 面试可展开
- 后续还能继续叠功能
