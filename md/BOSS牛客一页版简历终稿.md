# 赵云飞

求职意向：UE客户端开发实习生 / 游戏客户端开发实习生  
电话：13180902224 | 邮箱：645230806@qq.com | 作品链接 / GitHub：可补充

## 教育背景

哈尔滨师范大学 | 计算机科学与技术 | 本科 | 2022.09 - 2026.06（预计）

## 专业技能

- 熟悉 Unreal Engine Gameplay Framework，理解 GameMode、GameState、PlayerController、PlayerState、Character 在多人联机场景中的职责划分。
- 熟悉 Gameplay Ability System（GAS），能够基于 AttributeSet、GameplayAbility、GameplayEffect、GameplayTag 实现技能、属性、Buff / Debuff 与状态管理。
- 熟悉 UE 网络同步与服务端权威开发模式，了解 Dedicated Server、Session 创建 / 查找 / 加入、RPC 与属性复制等流程。
- 熟练使用 C++17 进行 UE 开发，掌握面向对象设计、智能指针、Lambda、委托、数据驱动等常用开发方式。
- 熟悉 UMG 与 C++ 事件通信，能够实现 UI 与 Gameplay 系统的低耦合交互。
- 具备 UE5 RDG、Global Shader、Scene View Extension 的实践经验，能够开发自定义后处理 Pass 与屏幕空间渲染调试插件。
- 熟悉 Git / P4V 协作流程，了解 Python、Flask、Docker、Linux 常用命令与基础部署方式。

## 实习经历

**九果一麦科技（UE独立游戏工作室） | UE程序开发实习生 | 2025.08 - 至今**

- 重构库存系统架构，拆分数据层与 UMG 表现层，降低界面与物品逻辑耦合，支撑商店、装备等模块迭代。
- 基于 DataAsset 与 DataTable 重构物品配置流程，搭建数据驱动的物品管理方式，提升新增与调整物品的效率。
- 开发基于 Editor Utility Widget 的库存调试工具，支持编辑器内查看、增删和修改物品属性，缩短联调与排查时间。
- 参与商店系统开发，处理商店、库存与角色属性之间的交互流程，在服务端权威下完成购买、出售与属性修改逻辑。

## 项目经历

**UE5 在线 MOBA 技术原型 | 独立开发 | 2025.04 - 2025.07**

- 基于 UE5 C++ 独立完成多人对战技术原型开发，覆盖 Gameplay Framework、GAS、UMG、AI、Session、Dedicated Server 与容器化启动链路。
- 搭建 GameMode、GameState、PlayerController、PlayerState、Character 的多人对局框架，实现角色生成、阵营分配、胜负结算与地图切换。
- 基于 GAS 实现属性成长、技能释放、Buff / Debuff、升级点与死亡流程，支持多段连击、AOE、投射物等战斗能力。
- 实现库存、商店、物品使用与合成系统，使用 DataAsset / DataTable 进行配置驱动，并通过 UMG 完成 HUD、商店、计分板等界面联动。
- 设计客户端建房到加入对局的联机流程：客户端请求协调器创建对局，协调器分配端口并拉起服务器实例，服务端创建 Session，客户端按 SessionSearchId 检索并加入对局。
- 编写 Python Flask 协调器与 Docker 部署配置，完成本地多实例服务启动与对局入口验证。

**UE5 RDG 渲染插件开发 | 独立开发 | 2026.03**

- 基于 UE5.5 C++、Render Dependency Graph（RDG）、Global Shader 与 Scene View Extension 实现自定义后处理渲染插件，搭建可扩展的屏幕空间 Pass 框架。
- 实现 Depth、Normal、Roughness、SceneColor 等调试视图，以及 Gaussian Blur、Bilateral Blur 等屏幕空间滤波效果，完成运行时可视化验证链路。
- 独立实现 SSAO 渲染流程，包括深度重建、半球采样核生成、遮挡估计、Normal-aware Bilateral Blur 降噪与 AO Composite 后处理合成。
- 使用 Slate 开发运行时调参窗口，支持多模式切换与 Blur / SSAO / AO Composite 参数实时调节，提高效果验证与问题定位效率。

## 获奖经历

- 2025 完美世界高校 MiniGame 开发大赛 最佳人气奖 《调和者》
- 2025 Godot 开发大赛 优秀奖 《[NULL] Protocol》

## 一页版排版建议

- 控制在 1 页，优先保证项目经历完整，删除年龄、性别、工作年限。
- 标题行只保留：姓名、求职意向、电话、邮箱、作品链接。
- 如果版面紧张，专业技能保留 5 到 6 条，把 Linux / Docker 合并进最后一条。
- 如果需要更偏图形学 / 渲染岗位，可把“UE5 RDG 渲染插件开发”放到项目经历第一位。
- 如果有 GitHub、演示视频、项目仓库地址，建议在标题区直接放链接，优先级高于年龄等信息。
