# RDG 入门名词解释 - 小白版

## 1. 先说结论：我们到底在做什么

你现在要做的，不是：

- 从零写一个图形引擎
- 从零实现 DirectX / Vulkan
- 一上来就研究复杂渲染论文

你要做的是：

**在 UE5 里做一个自己的渲染插件，把一段自定义图形学效果插进 UE 的渲染流程里。**

这句话里最重要的几个词，就是你现在不懂的这些：

- Project Plugin
- Scene View Extension
- Global Shader
- RDG

这份文档就是先把这些词讲明白。

## 2. 先建立一张总地图

把这个项目想象成一条流水线：

1. 你先做一个 **Plugin**
2. 插件里有自己的 **Module**
3. 模块注册一个 **Scene View Extension**
4. 当 UE 渲染画面时，这个扩展会在合适时机被调用
5. 你在那个时机里提交一个或多个 **RDG Pass**
6. 每个 Pass 里会跑你的 **Shader**
7. Shader 在 GPU 上处理屏幕数据，最后输出效果

所以这些词不是互相独立的，它们是串起来工作的。

## 3. Project Plugin 是什么

### 3.1 Plugin

**Plugin = 插件**

在 UE 里，插件就是一组独立的功能包。  
它可以包含：

- C++ 代码
- 蓝图节点
- 编辑器工具
- Shader
- 资源

你可以把它理解成：

**“插到项目里的一个独立功能模块包”**

### 3.2 Project Plugin

UE 里常见两种插件位置：

- Engine Plugin：放在引擎目录里
- Project Plugin：放在项目目录里的 `Plugins/`

我们选 **Project Plugin**，意思就是：

**这个插件属于当前项目，不改引擎源码。**

为什么选它：

- 更安全
- 更适合个人学习
- 不用动引擎源码
- 后续删掉、迁移都方便

### 3.3 你在这个项目里怎么理解它

对你来说，Project Plugin 就是：

**“这次图形学项目的载体”**

图形学内容本身不是 Plugin。  
Plugin 只是把你的代码和 Shader 正规地挂到 UE 里。

## 4. Module 是什么

### 4.1 Module

**Module = 模块**

UE 的 C++ 代码不是一整个大块，而是按模块组织的。  
每个模块通常会有：

- 模块名
- Build.cs
- 启动逻辑
- 依赖的其他模块

你可以把它理解成：

**“UE 工程里的一个代码分区”**

### 4.2 插件和模块的关系

一个插件里可以有一个或多个模块。

最简单的情况是：

- 1 个 Plugin
- 1 个 Runtime Module

这就够你做第一个渲染项目了。

### 4.3 为什么你要知道它

因为以后你会看到这些文件：

- `.uplugin`
- `YourPlugin.Build.cs`
- `IMPLEMENT_MODULE(...)`
- `StartupModule()`

这些全都属于“模块层”的概念，不是渲染算法本身。

## 5. Shader 是什么

### 5.1 Shader

**Shader = 跑在 GPU 上的小程序**

它负责处理图形数据，比如：

- 顶点怎么变换
- 像素最终显示什么颜色
- 纹理怎么采样
- 法线、深度怎么参与计算

你可以把它理解成：

**“显卡执行的代码”**

### 5.2 HLSL / USF

在 UE 里写 Shader，常见的是：

- HLSL 语法
- 文件扩展通常是 `.usf`

所以你以后看到的 USF 文件，本质上就是：

**“用 HLSL 风格写的 UE Shader 文件”**

### 5.3 为什么图形学项目一定离不开 Shader

因为真正的图形计算不是在普通 C++ 里做的，而是在 GPU 上做的。

C++ 更多负责：

- 准备参数
- 组织流程
- 告诉 UE 什么时候执行

Shader 负责：

- 真正计算效果

## 6. Global Shader 是什么

### 6.1 先说普通理解

在 UE 里，不是所有 Shader 都是同一种东西。

你已经可能接触过的，是：

- Material
- Post Process Material

这些都偏“材质系统”。

而 **Global Shader** 是另一类：

**它不是绑定在某个材质资产上的，而是由 C++ 直接驱动的 Shader。**

### 6.2 为什么叫 Global

因为它更像一种“全局渲染程序”，不是某个材质球专用。

它常用于：

- 全屏后处理
- 工具型渲染
- 自定义 Pass
- 计算着色器
- 插件内效果

### 6.3 它和 Material Shader 的区别

你可以先粗暴地这样理解：

- Material Shader：更偏内容制作、材质图形节点
- Global Shader：更偏底层渲染开发、C++ 控制

如果你是做图形学插件项目，Global Shader 更适合。

### 6.4 它在你项目里的作用

你做 SSAO、Blur、Debug View 这种效果时，真正跑在 GPU 上的代码，大概率就是一组 Global Shader。

## 7. Scene View 是什么

### 7.1 View

在渲染里，View 可以理解成：

**“这一次画面渲染所对应的观察视角信息”**

它里面通常有：

- 相机位置
- 相机方向
- 投影矩阵
- 视口大小
- 和这一帧画面相关的各种信息

### 7.2 Scene View

**Scene View = 场景视图的渲染描述**

简单说就是：

**“UE 正在准备画这一帧画面时，对当前视角的那份数据”**

你做屏幕空间效果时，很多信息都和 View 有关。

## 8. Scene View Extension 是什么

### 8.1 Extension

Extension 就是“扩展点”。

所以 **Scene View Extension** 的意思是：

**“给场景视图渲染过程插一个自定义扩展入口”**

### 8.2 它是干什么的

UE 本身已经有自己的渲染流程。  
你不能随便在任意地方插代码，不然流程会乱。

Scene View Extension 提供了一种官方方式，让你可以：

- 在渲染某一帧时拿到 View
- 在某些时机插入自己的渲染逻辑
- 不需要直接改引擎源码

### 8.3 为什么它重要

因为如果没有这个入口：

- 你的插件不知道什么时候参与渲染
- 你的 Shader 不知道什么时候执行
- 你的 RDG Pass 没地方挂

所以它可以理解成：

**“你进入 UE 渲染流程的门”**

## 9. Pass 是什么

### 9.1 Pass

**Pass = 一次渲染步骤**

一帧画面不是一步做完的，而是很多步骤拼起来的。

比如：

- 先画场景
- 再做光照
- 再做后处理
- 再做某个全屏效果

每一个步骤都可以看成一个 Pass。

### 9.2 Fullscreen Pass

**Fullscreen Pass = 对整张屏幕做一次处理**

比如：

- Blur
- Color Grading
- SSAO
- Edge Detection

这些很常见都是全屏 Pass。

对你来说，可以先理解成：

**“把屏幕当成一张图片，在 GPU 上做处理”**

## 10. Render Target 是什么

### 10.1 Render Target

**Render Target = 渲染输出到的目标纹理**

你可以把它理解成：

**“GPU 这一步算出来的结果，要写到哪张图上”**

有时候这张图是最终屏幕。  
有时候只是中间结果。

### 10.2 为什么需要中间图

因为很多效果不是一步完成的。

例如 SSAO：

- 第一步先算 AO 原始结果
- 第二步再 Blur
- 第三步再和场景颜色混合

那么中间图就必须先存下来。

## 11. Render Thread 是什么

### 11.1 Game Thread

你平时写的很多 UE 逻辑，主要跑在 Game Thread。

比如：

- Actor 逻辑
- UI 逻辑
- 输入
- Gameplay

### 11.2 Render Thread

**Render Thread = 负责组织渲染命令的线程**

它不是 GPU 本身，但它负责把渲染工作准备好、提交给底层图形接口。

### 11.3 你为什么要知道它

因为图形学项目里经常会碰到：

- 这段代码是不是在渲染线程跑
- 某些对象能不能在这个线程访问
- 某些资源是不是只该在渲染阶段创建

现在你不用深究线程细节，但要有这个意识：

**渲染代码和普通游戏逻辑代码，不一定跑在同一个线程。**

## 12. RHI 是什么

### 12.1 RHI

**RHI = Render Hardware Interface**

它是 UE 对底层图形 API 的抽象层。

底层真实 API 可能是：

- DirectX 12
- Vulkan
- Metal

而 UE 不希望你每次都直接写这些 API，于是中间做了一层抽象，这就是 RHI。

### 12.2 你现在怎么理解它

先把它理解成：

**“UE 和底层显卡 API 之间的适配层”**

你做 RDG 时会接触到一些 RHI 相关类型，但第一阶段不用把它研究透。

## 13. RDG 是什么

### 13.1 RDG 全称

**RDG = Render Dependency Graph**

中文通常叫：

- 渲染依赖图

### 13.2 它到底在解决什么问题

如果没有 RDG，渲染代码会很容易变成：

- 手动管理很多临时纹理
- 很难看清哪个 Pass 依赖哪个 Pass
- 很容易资源生命周期混乱
- 很难优化和维护

RDG 的作用就是：

**帮 UE 以“图”的方式组织渲染资源和 Pass 依赖关系。**

### 13.3 用人话理解

你可以把 RDG 理解成一个“调度员”。

你告诉它：

- 我要哪些中间资源
- 我要哪些 Pass
- 这个 Pass 读什么，写什么

它来帮你整理：

- 执行顺序
- 资源生命周期
- 依赖关系

### 13.4 重要认知

RDG 不是某种效果算法。  
它不是 SSAO，不是 Blur，不是后处理本身。

它只是：

**“组织渲染效果执行的方式”**

## 14. RDG Pass 是什么

### 14.1 RDG Pass

当你用 RDG 时，通常会向图里添加一个个 Pass。

每个 Pass 都会说明：

- 这个 Pass 读哪些资源
- 这个 Pass 写哪些资源
- 这个 Pass 跑什么 Shader

### 14.2 你可以怎么理解

如果说 RDG 是调度员，  
那么 RDG Pass 就是它安排的一项项具体任务。

例如：

- Pass A：生成 AO 原图
- Pass B：对 AO 做 Blur
- Pass C：输出 Debug View

## 15. 这些词在你的项目里分别扮演什么角色

把它们放回同一条链路里看：

- **Project Plugin**
  你的项目载体，负责把功能独立放到插件里

- **Module**
  插件里的代码组织单元，负责启动和注册逻辑

- **Scene View Extension**
  你进入 UE 渲染流程的入口

- **Global Shader**
  真正在 GPU 上执行图形计算的程序

- **RDG**
  组织资源和 Pass 的系统

- **RDG Pass**
  一步步具体执行的渲染任务

你可以记成一句话：

**插件负责挂进去，扩展负责接进渲染流程，RDG 负责组织步骤，Shader 负责真正算效果。**

## 16. 你现在必须先补的前置知识

先学这些，够用了：

### 16.1 UE 工程基础

- Plugin 是什么
- Module 是什么
- Build.cs 是什么
- `.uplugin` 是什么

### 16.2 Shader 基础

- Shader 是什么
- HLSL 基本语法
- 纹理采样
- UV
- 全屏 Pass 的基本思路

### 16.3 图形学基础

- 向量和矩阵
- 深度缓冲
- 法线
- 屏幕空间效果是什么

### 16.4 UE 渲染基础名词

- Render Thread
- RHI
- RDG
- Global Shader
- Scene View Extension

## 17. 你现在先不要学太深的东西

先别急着啃这些：

- Vulkan
- DX12 细节
- Lumen
- Nanite
- 大量引擎源码
- 复杂渲染论文

因为你现在的目标不是“成为图形学研究员”，而是：

**先把 UE 渲染插件这条链路打通。**

## 18. 建议你现在的学习顺序

按这个顺序最稳：

1. 先弄懂 Plugin 和 Module
2. 再弄懂 Shader 和 Global Shader
3. 再弄懂 Scene View Extension 是接入口
4. 最后理解 RDG 是怎么组织 Pass 的

这个顺序的关键点是：

不要一上来就死磕 RDG。  
因为如果你连插件怎么进 UE、Shader 怎么跑都没概念，直接看 RDG 会非常抽象。

## 19. 你看完这篇后，至少要能回答这几个问题

如果你能回答下面这些，说明入门认知差不多建立起来了：

- Plugin 和普通游戏代码有什么区别？
- Scene View Extension 为什么是“入口”？
- Global Shader 和 Material Shader 有什么大区别？
- RDG 为什么存在？
- Pass 和 RDG Pass 是什么？
- 这个项目里 C++ 和 Shader 各负责什么？

## 20. 最后一句话总结

这次项目的本质，不是“学一个叫 RDG 的名词”，而是学会这条链路：

**在 UE5 里，用插件接入渲染流程，用 Scene View Extension 找到入口，用 RDG 组织 Pass，用 Global Shader 在 GPU 上实现图形学效果。**
