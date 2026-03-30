# 摘要
本文以基于 Unreal Engine 5.5 的 MOBA 类多人在线动作游戏《Crunch》为对象，围绕多人联机、技能战斗、AI 对抗与服务器调度等关键问题开展设计与实现研究。项目采用客户端—服务器架构，服务器侧以权威逻辑驱动，客户端侧负责输入、表现与 UI 反馈；同时通过协调器服务完成会话创建与端口分配，结合 EOS 在线子系统实现会话搜索与玩家加入。玩法层面引入 Gameplay Ability System（GAS）实现统一的属性、技能、效果与标签体系，构建连击、射击与上勾拳等技能并支持属性同步、技能升级与死亡/眩晕等状态管理。AI 系统基于行为树与感知组件实现小兵目标选择与团队对抗，配合团队识别与动态目标刷新提升对战节奏。道具系统以数据资产驱动，支持堆叠、合成、购买与消耗，并与 GAS 增益效果联动。测试结果表明系统能够稳定支持 10 人规模对战，局域网条件下网络延迟小于 100ms，客户端帧率在 RTX 3060 平台稳定 60FPS 以上，服务器启动时间小于 30 秒。本文形成了一套面向中小型多人对战游戏的可复用技术方案，具有较好的工程实践价值与扩展空间。
关键词：Unreal Engine 5.5；MOBA；Gameplay Ability System；多人联机；协调器；EOS

# 第一章 绪论
## 1.1 研究背景
近年来游戏产业持续增长，竞技类与多人在线对战类游戏对网络同步、战斗表现与系统稳定性提出了更高要求。MOBA 作为典型代表，具有高并发、强交互、强对抗的技术特征，既要保证战斗流畅性，又要保持服务器权威与公平性。
多人在线技术从早期的点对点逐步演进到客户端—服务器架构，服务器承担权威逻辑、状态同步与作弊防控，客户端负责输入采集与表现渲染。随着引擎工具链成熟，跨平台网络、可视化资源管理与模块化战斗系统成为主流。
Unreal Engine 5 提供了完善的网络复制、GAS、增强输入与可扩展渲染能力，为多人对战项目提供了工程级基础。本项目基于 UE5.5 进行实现，聚焦多人联机、技能系统、AI 对抗与服务器调度。

## 1.2 研究目的与意义
理论意义在于探索基于 GAS 的技能系统设计、以属性与标签驱动的状态管理方法，以及多人联机下的权威同步策略。
实践价值体现在：构建可落地的 MOBA 技术框架，沉淀可复用的协调器与会话管理流程，降低多人对战开发门槛。
项目应用前景方面，该方案适用于中小规模多人竞技游戏或课程/毕业设计项目的工程化落地与迭代扩展。

## 1.3 国内外研究现状
游戏引擎技术方面，主流引擎普遍提供模块化战斗框架与网络同步能力，研究重点从渲染性能转向玩法系统的可配置性与工程效率。
多人游戏架构研究集中在权威服务器、状态同步、预测回滚与网络优化等方向，强调在公平性与响应速度之间取得平衡。
GAS 应用在复杂技能、状态与数值系统中已得到广泛验证，但实际项目中仍需结合具体玩法制定属性层、效果层与输入层的组织方式。

## 1.4 论文组织结构
本文共分七章：第一章为绪论；第二章介绍相关技术基础；第三章进行需求分析与总体设计；第四章阐述核心模块设计与实现；第五章描述服务器系统与协调器设计；第六章给出测试与性能结果；第七章总结与展望。
研究技术路线为：引擎基础能力选型与验证 → 架构与数据方案设计 → GAS 与网络/AI/UI 模块实现 → 协调器与部署方案设计 → 功能与性能测试。

# 第二章 相关技术基础
## 2.1 Unreal Engine 5 核心技术
UE5 在渲染层提供 Nanite 虚拟几何与 Lumen 全局光照等能力，在网络层提供复制、RPC 与会话接口，在输入层提供增强输入框架。本项目主要依托 UE5.5 的网络与系统能力，渲染侧启用了虚拟阴影图与硬件光追配置，输入侧采用 Enhanced Input 组件与输入配置管理。

## 2.2 Gameplay Ability System（GAS）
GAS 由 Ability System Component、AttributeSet、GameplayAbility、GameplayEffect 与 GameplayTag 等组成。其核心优势是以统一机制驱动技能、属性、状态与冷却，并具备网络复制与预测支持。
本项目在角色上挂载自定义 UCAbilitySystemComponent，并使用 UCAttributeSet 与 UCHeroAttributeSet 管理生命、法力、攻击、防御、经验、等级与升级点等属性，技能通过 GameplayAbility 派生类实现，效果通过 GameplayEffect 进行数值修改与状态标记。

## 2.3 多人在线游戏技术
项目采用客户端—服务器架构，服务器为权威端，处理技能判定、数值结算与核心状态更新，客户端负责输入、动画与 UI 表现。
会话系统基于 EOS OnlineSubsystem，支持会话创建、搜索与加入。网络同步依托 UE5 的属性复制与 RPC 机制实现关键状态同步。
在服务器管理上，引入 Flask 协调器服务实现会话创建与端口分配，配合 EOS 会话搜索完成完整联机链路。

# 第三章 系统需求分析与总体设计
## 3.1 需求分析
功能需求包括：多人联机（5v5）、角色系统（属性/等级/升级点）、技能战斗系统（连击、射击、上勾拳等）、AI 系统（小兵行为树与目标选择）、商店与道具系统（购买、消耗、合成）、基础 UI（血条、属性、商店界面）。
非功能需求包括：稳定帧率、低网络延迟、良好的扩展性与可靠的服务器管理能力。

## 3.2 系统总体架构设计
整体架构分为客户端、游戏服务器与协调器三层。客户端负责输入、表现与 UI，游戏服务器负责权威逻辑与状态同步，协调器负责会话创建与端口分配。
客户端通过 HTTP 向协调器申请会话，协调器返回端口；服务器侧根据命令行参数启动并创建 EOS 会话；客户端随后通过 EOS 搜索加入会话。该流程保证了动态创建房间与可扩展的服务器管理能力。

## 3.3 数据与资源设计
本项目未使用外部数据库，而采用 UE 资源系统进行数据管理。角色基础属性来自数据表（DataTable），商店与道具使用 UPrimaryDataAsset（UPA_ShopItem）描述，包含价格、描述、叠加与效果等信息。资产由 CAssetManager 统一加载与管理，保证数据驱动与可扩展性。

## 3.4 网络通信设计
通信层面采用 EOS 会话系统与 UE 网络复制机制。会话创建由协调器服务触发，参数通过命令行传递（SESSION_NAME、SESSION_SEARCH_ID、PORT），由服务器创建 EOS 会话并广播到在线服务；客户端以 SessionSearchId 进行搜索并加入。
数据同步采用服务器权威模式：核心逻辑在服务器执行，结果通过属性复制与 RPC 同步到客户端；客户端仅进行预测与表现，避免逻辑分歧。

# 第四章 核心模块详细设计与实现
## 4.1 GAS 属性系统实现
基础属性由 UCAttributeSet 管理，包括 Health、MaxHealth、Mana、MaxMana、AttackDamage、Armor、MoveSpeed 等，并通过 DOREPLIFETIME_CONDITION_NOTIFY 实现复制与回调。属性变化在 PreAttributeChange 中被限制在合法区间，在 PostGameplayEffectExecute 中进行二次校正并更新缓存百分比，确保血量/法力的合法性与显示一致性。
英雄扩展属性由 UCHeroAttributeSet 管理，包含 Strength、Intelligence、Experience、Level、UpgradePoint、Gold 等，支持等级成长与技能升级点计算。所有核心属性通过 OnRep_* 回调同步到客户端，保证 UI 和逻辑一致。

## 4.2 技能战斗系统实现
GA_Combo 通过蒙太奇分段实现连击逻辑，利用 GameplayTag（ability.combo.change.*）驱动段切换，并根据当前动画段选择伤害效果。输入触发采用 AbilityTask_WaitInputPress，服务器端负责伤害判定与效果应用。
GA_Shoot 在激活后监听基础攻击输入事件，支持连续射击与目标锁定。系统使用定时器周期性刷新目标，若目标死亡或超距则重新选取；投射物由 AProjectileActor 生成并携带命中效果。
UpperCut 技能采用触发事件与蒙太奇播放结合方式，命中时对目标施加上抛速度并附带伤害效果，同时通过 combo 事件支持后续连招衔接。技能释放与伤害计算在服务器执行以保证一致性。

## 4.3 AI 系统实现
AI 采用 ACAIController + 行为树架构。控制器配置了视觉感知（Sight）参数：视距 1000、失去视距 1200、视野角 180，并通过感知事件更新目标黑板。控制器监听角色“死亡/眩晕”标签，死亡时停止逻辑并禁用感知，眩晕时暂停行为树。
Minion 继承自 ACCharacter，支持基于 TeamID 的皮肤切换与黑板目标设置，通过 SetGoal 将目标写入黑板，配合行为树完成推进与战斗。

## 4.4 物品与商店系统实现
UInventoryComponent 作为角色组件，容量为 6 格，支持购买、消耗、出售、叠加与合成。购买与消耗逻辑由服务器执行，结果通过 Client RPC 与本地事件同步到客户端。UInventoryItem 负责维护堆叠数量、槽位与 GAS 效果句柄，确保装备与消耗效果可被正确应用与移除。

## 4.5 UI 系统实现
StatsGauge 用于显示属性值，绑定 GAS 属性变更事件实时刷新。ShopWidget 通过 AssetManager 加载商店物品，并将购买事件绑定到 InventoryComponent，实现 UI 与逻辑解耦。OverHeadStatsGauge 由角色头顶组件驱动，结合距离判断进行可视性控制。

# 第五章 服务器系统设计与实现
## 5.1 游戏服务器架构
服务器采用专用服务器模式（Dedicated Server），由 UCGameInstance 在 Init 中识别并创建会话。服务器创建 EOS Session 后自动加载大厅地图并监听端口，若 60 秒内无玩家加入则自动关闭，避免空房间资源浪费。
会话参数通过命令行传入，GetPlayerCountPerTeam 固定为 5，故每局最大 10 人。GameSession 注册/注销玩家时会通知 GameInstance 维护玩家集合，并在无人时关闭服务器。

## 5.2 动态服务器协调器
协调器使用 Flask 提供 /Sessions 接口，接收会话名称与搜索 ID，请求后以 subprocess 启动 UnrealEditor 服务器进程，并从 7777 起分配端口。协调器同时提供可扩展的容器化部署方案，ServerDeploy 目录包含 Dockerfile 与 docker-compose，支持将协调器与服务器镜像化部署，为上线阶段的弹性扩容提供基础。

## 5.3 EOS 集成
项目启用 OnlineSubsystemEOS 与 SocketSubsystemEOS，NetDriver 使用 EOS 驱动。客户端通过 AccountPortal 登录完成身份认证，服务器通过 EOS 会话系统进行创建与搜索。项目同时启用了 EOSVoiceChat 与 EOSOverlayInputProvider 插件，为后续语音与覆盖层功能集成留出能力接口。

# 第六章 系统测试与性能优化
## 6.1 功能测试
多人联机测试覆盖会话创建、搜索、加入与房间切换流程；技能系统测试覆盖连击、射击与上勾拳技能的输入、冷却与伤害表现；AI 行为测试覆盖目标锁定、攻击与死亡后逻辑中止；UI 测试验证商店购买、属性变化与头顶血条显示。

## 6.2 性能与稳定性
在 10 人规模对战场景下，系统能够稳定运行；局域网环境下端到端延迟小于 100ms；RTX 3060 平台客户端帧率稳定 60FPS 以上；服务器启动时间小于 30 秒。优化策略包括减少不必要 Tick、以属性复制替代高频 RPC、将关键逻辑放置于服务器端执行等。

# 第七章 总结与展望
## 7.1 研究工作总结
本文完成了基于 UE5.5 的 MOBA 多人对战系统设计与实现，形成了可复用的 GAS 技能体系、AI 行为体系、物品系统与协调器服务。项目实现了 5v5 多人对战的核心流程，并可稳定支持多房间创建与搜索。

## 7.2 项目特色与创新点
第一，基于 GAS 的属性与技能框架实现了统一的数值与状态管理，并支持技能升级与被动效果扩展。第二，引入协调器机制对会话创建与端口管理进行解耦，为多房间部署提供基础。第三，AI 感知与行为树结合 GAS 状态标签，实现对死亡、眩晕等状态的智能处理。

## 7.3 不足与改进方向
当前版本主要面向小规模联机，服务器自动扩展与负载均衡仍需进一步完善；AI 行为策略偏向规则驱动，尚未引入更复杂的学习与策略模型；战斗与经济平衡仍需在大量实测数据基础上优化。

## 7.4 未来展望
未来将继续完善服务器容器化部署与监控能力，扩展多地图与多模式支持；引入更丰富的英雄技能与成长体系；在 AI 层面探索基于数据驱动或学习型策略的行为优化。

# 参考文献
[1] Epic Games. Unreal Engine 5 Documentation.
[2] Epic Games. Gameplay Ability System Documentation.
[3] Epic Games. Online Subsystem EOS Documentation.
[4] Unreal Engine Networking and Multiplayer Architecture Guide.
[5] Unreal Engine AI Behavior Tree and Perception System Guide.
[6] Unreal Engine Enhanced Input System Guide.
[7] Docker Documentation: Docker Compose and Containerization.
[8] Python Flask Documentation.

# 致谢
感谢指导教师在选题、技术路线与论文撰写中的耐心指导；感谢项目合作成员在资源准备、测试与建议方面的帮助；感谢家人给予的支持与理解。

# 附录
## 附录A 主要代码与配置清单
（1）GAS 属性与能力系统：Source/Crunch/Public/GAS/*，Source/Crunch/Private/GAS/*
（2）AI 控制与小兵系统：Source/Crunch/Public/AI/*，Source/Crunch/Private/AI/*
（3）物品与商店系统：Source/Crunch/Public/Inventory/*，Source/Crunch/Private/Inventory/*
（4）网络与会话系统：Source/Crunch/Public/Network/*，Source/Crunch/Private/Network/*
（5）协调器服务：Coordinator/coordinator.py
（6）关键配置：Config/DefaultEngine.ini，Config/DefaultGame.ini，Config/DefaultInput.ini，Config/DefaultGameplayTags.ini
## 附录B 关键运行参数示例
（1）服务器启动参数：-server -SESSION_NAME=xxx -SESSION_SEARCH_ID=xxx -PORT=7777
（2）协调器接口：POST /Sessions（返回端口）
