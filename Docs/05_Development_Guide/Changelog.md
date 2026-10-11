# Moth Effect（飞蛾效应）：变更日志

版本 v0.36 · 2026-10-11 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件按日期追加文档与项目变更，保留每次操作当时的实际范围和验证状态。2026-10-03 的未编译/未提交记录是历史，不覆盖后续远程配置、合并和任务状态。当前文档集为 v0.32，开发状态见 [开发计划](Development_Plan.md)，修改顺序见 [文档维护流程](Documentation_Workflow.md)。

## 1. 变更记录

| 变更 ID | 日期 | 内容 | 原因 | 影响与验证 |
| --- | --- | --- | --- | --- |
| CHG001 | 2026-10-02 | 建立 v0.1 文档基线 | 把创意转换为可执行设计 | 尚未实现，需 G0 至 G4 验证 |
| CHG002 | 2026-10-02 | 确认 UE 5.8 与混合开发，无现成资源 | 用户补充 | [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 使用混合架构，[关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 采用模板与自建灰盒 |
| CHG003 | 2026-10-02 | 更新 v0.2，确认游戏名称与魔法朋克风格（现行名称：Moth Effect / 飞蛾效应） | 用户确定名称与风格 | 更新标题、表现提案和元数据，战斗数值未改 |
| CHG004 | 2026-10-02 | 记录既有模板工程与 MothEffect 模块，复用现有角色/控制器/游戏模式 | 用户创建工程，本次只读核对 | 更新 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)/[开发任务与排期](Development_Plan.md)，T02 标部分完成；编译与打包未执行 |
| CHG005 | 2026-10-02 | 整理 v0.3 文档：统一英文语义文件名、阶段归档、拆分职责、更新导航与阅读版 | 用户要求按已有文档结构规范管理 | 保留所有规则/任务/测试/决策 ID，参数数值未改；澄清 TC29 本地验收与平台自测的执行顺序，校验文件链接与内容保留 |
| CHG006 | 2026-10-03 | 初始化本地 main、连接 GitHub origin、配置 .gitignore 与 Git LFS、补充公开建议 | 用户要求连接仓库并明确禁止提交推送 | 未暂存/提交/推送；T03 标部分完成，恢复仍待验收；标记个人路径与配置凭证字段，未输出凭证值、未修改游戏实现或引擎配置 |
| CHG007 | 2026-10-03 | 将个人磁盘路径改为相对链接/路径，同步 HTML，处理 AndroidFileServer Token | 用户准备自行提交推送，要求清理公开内容 | JSON 路径基准为其文件目录，玩法参数未改；禁用 AFS 用途和网络连接、Token 显式清空；未暂存/提交/推送 |
| CHG008 | 2026-10-03 | Content 下只将 MothEffect 子目录纳入 Git，其余目录和顶层文件排除 | 用户指定提交范围 | 本机资源未删除，LFS 继续用于保留资产；记录当前 ThirdPerson 等依赖不会随仓库分发的限制；未暂存/提交/推送 |
| CHG009 | 2026-10-03 | 统一全部文档游戏名：英文 Moth Effect，中文飞蛾效应 | 用户纠正正式写法并补充中文名 | 更新仓库 README、Markdown、JSON 名称元数据及 HTML 阅读版为 v0.7；工程/模块/路径/类名与玩法参数未改；未暂存/提交/推送 |
| CHG010 | 2026-10-03 | 新增 [候选机关道具](../02_Design_Doc/GDD/Device_Candidates.md)：牵引核心、诱光灯、折光棱镜、静滞锚、换位匣 | 用户要求保存为待选制作道具 | 记录效果、组合、待验证边界与观测目标，同步策划/规则入口/决策/开发计划/HTML；未新增正式任务、数值或实现，D01–D03 首版范围不变 |
| CHG011 | 2026-10-03 | 确认 v0.9 玩家方案：首版无 GAS，复用本地 Rifle 动画，C++ 行动状态/AnimBP 移动状态/上半身 Montage；补齐职责、转换、输入归属、换弹/拾取、动画中断及后续扩展边界 | 用户明确确认上一轮方案并要求补齐文档 | 更新 REQ09/REQ10、R19–R23、DEC22–DEC27，新增 TC34–TC38；同步资源/配置核对/命名/计划/入口/HTML；新增 aimMoveSpeedCmPerSec 与 airControl，其他数值不变且全部未实测；T04/T05/T09 粗估与总工时同步，玩法任务/测试仍未完成 |
| CHG012 | 2026-10-03 | 开始 T04：写入越肩相机、镜头 Yaw 朝向、瞄准 FOV、冲刺互斥与输入 Flush 清理；新增 [玩家第一步接入](Player_Setup.md) | 用户选择助手改 C++、本人配置 UE 资产和试玩；随后明确不由助手编译 | 4 个源码文件修改，暴露 Aim/Sprint 引用与动画读状态；T04 标进行中，输入/动画资产待用户接入；修改后未编译，所有运行测试仍未执行；同步进度与阅读快照 |
| CHG013 | 2026-10-03 | 补充 ABP 的 IsAiming 数据同步、Direction/GroundSpeed、基础腾空与 AimOffset 混合步骤，同步实际输入/动画目录 | 用户报告完成输入、复制 ABP、八方向 BlendSpace 和 IsAiming 变量，要求继续制作 | 复用现有 C++ getter 与 UAnimInstance；静态核对资产存在及 BlendSpace 命名，不推断内部接线或试玩成功；T04 继续进行中，运行测试仍未执行；助手未编译或修改资产，更新阅读快照 |
| CHG014 | 2026-10-03 | 开始 T05：ARifle 双段射击/弹匣/手动换弹、生命与命中接口、行动状态和 Montage 入口；新增 Weapon_Setup，补记用户移动速度代码调参与 AO -1～1 归一化步骤 | 用户报告本轮测试成功并询问下一步，沿已确认协作方式继续制作 | 用户此前只改代码的移动速度调参保留，JSON 未同步且已标明差异；枪械/生命默认参数沿用账本。T04 局部成功由用户报告，T05 进行中；新代码未编译/运行，资产配置和武器验收待用户完成；同步文档链接/阅读快照 |
| CHG015 | 2026-10-03 | 新增角色 Mesh 下的 RifleComponent，使用完整 BP_Rifle 子 Actor 预览/持枪；移除独立 SpawnActor 与旧偏移字段，保留类型/挂点设置并完善绑定/清理 | 用户反馈原方案无法方便地在角色蓝图预览和调整位置 | 组件 Transform 成为偏移来源，旧偏移需手动迁移；类不变时不重新设置 ChildActorClass；组件负责武器生命周期。复核本地 UE 5.8 API 与源码，未编译/预览/试玩，未修改资产；更新接入文档与阅读版 |
| CHG016 | 2026-10-06 | 建立 M1–M6、27 条 T01–T27 Issue 和首版开发 Project，确认 10/17 DDL | 用户要求用 gh 落地任务管理 | 已核对阶段截止与 Issue/任务对照；T01–T03 为 Done，T04 仍为 Pending Acceptance，T05 为 In Progress；记录不足不自动补作完整验收 |
| CHG017 | 2026-10-06 | 配置四个 Project 视图与轻量 Repository checks | 用户要求补 Views 与 Actions | [PR #30](https://github.com/YYchainsAw/MothEffect/pull/30) 已合入 main/develop，PR 及两分支 CI 通过；静态检查不编译/运行 UE |
| CHG018 | 2026-10-06 | 配置开发任务、Bug 和默认 PR 模板 | 用户要求配置 Issue/PR 模板 | [PR #31](https://github.com/YYchainsAw/MothEffect/pull/31) 已合入 main/develop，GitHub 已识别模板，CI 通过；Refs 关联任务，验收与关闭分别记录 |
| CHG019 | 2026-10-06 | 同步 v0.14：入口、六阶段排期、27 条 Issue 对照、当前状态、GitHub 流程及阅读快照，补充快照生成/离线核对脚本 | 用户要求 Docs 同步 | 保留既有 T/G/TC/DEC/CHG ID 和玩法数值；T04 Closed / Pending Acceptance 与缺失记录如实归档，完整游戏验收状态保持待证据；文档链接、快照源摘要/锚点及静态检查验证见对应 PR |
| CHG020 | 2026-10-07 | 核对本机/远程源码、9 个自有资产、日志与 27 条 Issue；确认每天 4–6 小时；为 UI、少量原创机关/场景资产与必要反馈预留工时；修订剩余阶段目标，保持 10/17 DDL | 用户要求检查已完成部分并在开发计划安排 UI/美术，随后确认时间与制作目标 | 新增进度核查，记录 T06/T09 前置及 UI/反馈可复用接口；T18 4–6 小时、T20 6–8 小时，总/剩余预算与 55 小时日程同步；保留原验收勾选和关闭记录，不新增游戏验收通过；同步资源规格/决策/入口/HTML，未编译、运行、打包或修改玩法代码/参数 |
| CHG021 | 2026-10-07 | 记录 T05 动画/当前边界 PIE 由用户确认通过、PR #34 已合并和 Issue #7 Closed / Done；新增 T06 DeviceBase/状态枚举、统一命中激活、拥有者状态提交、碰撞与结束清理、三项自动化测试及 Device_Setup；同步任务/测试/入口/阅读快照，记录当前直接在 develop 开发 | 用户确认 T05 完成后要求继续完成 T06，并明确禁止助手自行增加分支 | 复用 FHitContext/IBallisticReactive，先 Active 后回调/效果，重复命中不覆盖首次证据；参数未改；T06 #8 保持 Open / In Progress。助手核对本机 UE 5.8 源码并执行静态检查，未编译/运行 UE、修改二进制资产或提交/推送；用户完整编译、Automation 和 PIE 待执行，Held 实际操作及效果/容量随后续任务补测 |
| CHG022 | 2026-10-07 | 记录用户 T06 Automation Run 3 两项通过、一项失败；修正 DeviceActivationTests 的 World Context、InitializeActorsForPlay 与 EndPlay 清理，增加初始化检查和首次接口失败时停止后续断言；同步接入/测试/进度记录与阅读快照 | 用户提供首次接口激活失败及 World has no context 日志；本机 UE 5.8 Actor::ProcessEvent 与 UHT 代码核对确认测试环境缺失初始化 | 保留 Execute 接口验证，玩法状态与激活源码未改；用户已提交基础版本 3b0e93f，本轮修正待用户重新编译、运行三项 Automation 与 PIE；助手仅静态核对，不新增通过结果，不提交/推送或创建分支 |
| CHG023 | 2026-10-07 | 将 DeviceActivationTests 中误写的 Engine/URL.h 修正为 Engine/EngineBaseTypes.h，更新接入记录与阅读快照 | 用户报告 MSB3073 / code 6；本机 UBT 日志确认 C1083，UE 5.8 的 FURL 定义实际位于 EngineBaseTypes.h | 核对测试文件 9 个引号 include 均存在，静态差异检查通过；用户已提交前次修正 8085801，本次一行头文件修正待用户重新编译和 Automation 复测；助手未执行 UE 编译 |
| CHG024 | 2026-10-08 | 归档 T06 构建检查成功、Automation Run 4 三项 Success 及用户明确确认的 PIE 激活/重复命中/结束状态分支；保存可提交的原始日志摘录，更新接入/测试/进度和阅读快照，按模板准备报告 | 用户确认三项 Automation 与上述 PIE 项均通过，并要求本人操作 PR、助手仅提供报告 | 日志执行日期为 2026-10-07，归档日期为 10/8；当前实现源码为 c0a7589，保留此前失败历史；当前 T06 范围通过，完整 TC12 随 T07/T08/T13 补测；本轮仅本地归档与静态检查，未代为编译、提交/推送、创建 PR 或修改 GitHub 状态 |

## 2. 本轮归档说明

| 后续变更 ID | 日期 | 内容与原因 | 影响与验证 |
|---|---|---|---|
| CHG025 | 2026-10-08 | 用户确认 T07 六项 Automation 与基础拾取/放下/投掷 PIE 通过；随后明确其他边界目前无法测试，归档局部结果与模板报告 | 保留历史增量构建结果和真实通过范围；完整边界仍待执行，见 [T07 接入](Device_Interaction_Setup.md) 与 [原始日志](../06_Test_Doc/Evidence/T07_Verification_2026-10-08.log) |
| CHG026 | 2026-10-08 | 用户要求直接继续 T08，并授权每完成小功能本地提交；P01 `35643dc`、D03 `e969ac3`、蓝图默认半径/缩放出生检查加固 `8ee90e6`，接入统一 Sweep 弹道、世界方向提示、计划发射、出生净空和结束清理，新增六项 Automation 源码 | 参数 v0.10 新增未实测几何/视觉默认值；继续在 develop 开发，无新分支、推送、PR 或 GitHub 状态变更。助手仅源码/API 复核与静态检查，UE 构建、Automation、PIE/G1 全部待执行，见 [T08 接入](Emitter_Setup.md) 与 [模板报告](../06_Test_Doc/T08_Report_2026-10-08.md)；T07 边界不据此通过 |
| CHG027 | 2026-10-08 | 玩家迁移至 Murdock：单 Jog 速度与参数同步，取消走路/冲刺切换；一体枪使用角色枪口 Socket，隐藏独立枪且保留原 Rifle 玩法；补充 [迁移指南](Murdock_Setup.md) 与外部枪口回归测试源码 | 用户要求人物/枪械一体接入、修正左键无效果并统一 Jog；旧 WeaponSocket 校验会阻止绑定，玩家资产仍引用 Mannequin Montage | 仅代码、参数与文档修改；二进制资产由用户配置，助手未编译/运行 UE，Automation/PIE 待验收；不据旧模板结果标记新模型通过 |
| CHG028 | 2026-10-11 | 收尾 T09：新增九项原生玩家边界测试，复现并修正无 Flush 应用失焦续射与 UnPossessed 后旧角色续射，清理解除控制时的换弹/恢复/Held；初始化释放查询输出以消除 C4701 | 用户要求收尾 T09 / #11；当前 Issue 前三项已勾选，末项与验证记录未完成 | 助手实际执行 UE 5.8 Editor 增量构建与全量 23 项 Automation，全部 Success、未预期 errors/warnings 为零；保留首轮两项失败。玩法参数和用户资产未改，真实 Murdock/窗口/整局重开仍待证据。源码与文档本地提交，未推送或修改 GitHub，见 [T09 报告](../06_Test_Doc/T09_Report_2026-10-11.md) |

T05 接入补记（2026-10-03）：用户已创建 `/Game/MothEffect/Weapons/BP_Rifle`，并在 `hand_r` 下添加 WeaponSocket；C++ 默认挂点与 Weapon_Setup 已同步，已有角色蓝图仍需核对挂点字段。助手未修改资产或执行编译/试玩。

CHG027（2026-10-08）：用户提供 T08 Devices Test Run 3，九项 Success，三个 Emitter 案例带缺少 World EndPlay 警告；读取并归档本机原始日志与 UBT 的 up-to-date 成功检查。提交 `23eb829` 用 UWorld::EndPlay 配对世界/Actor/Subsystem 收尾，修正后编译与复测待用户执行；同步 [接入步骤](Emitter_Setup.md)、[报告](../06_Test_Doc/T08_Report_2026-10-08.md) 与 [验证日志](../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。P01 三项、PIE 和 G1 仍待验证；助手没有运行 UE，用户资产改动保留。

阅读指南归入项目概览；游戏策划、道具规则、关卡/界面/资源和参数归入 GDD；UE 实现设计归入 TDD，并拆出代码规范、版本管理和工程配置；测试与提交拆为测试计划、发布清单；决策与变更拆为设计决策、维护流程和本变更日志。根目录 README 提供有效文件索引，阅读版同步为新结构。

CHG028（2026-10-08）：归档用户 17:06 最新 Test Run 3，Devices 九项与 Projectiles 三项全部 Success，World 清理警告不再出现。保留 16:43 的旧警告历史与两轮 UBT up-to-date 检查，更新 T08 报告、接入和任务/测试状态；本轮仅记录实际结果与同步阅读版，未更改玩法源码或用户资产。PIE TC03–TC05、真实暂停和 G1 仍待验收；助手未运行 UE。

早期 v0.1/v0.2 文件包是历史快照；当前有效基线以项目 Docs 为准。

CHG029（2026-10-08）：用户报告 PIE 中没有发射方向提示；核对 UE 5.8 Arrow 视图相关渲染条件和旋转矩阵，修订接入为 EmitterDirectionMarker 下的 DirectionVisual 锥体网格，明确局部 Pitch=-90°、相对挂接、材质/显隐和激活后显示。原生方向/显隐及玩法源码未改，十二项 Automation 通过记录保留；配置后的 PIE 效果待用户确认。助手未修改资产或运行 UE。

CHG030（2026-10-08）：用户随后确认方向显示通过并询问 PR；归档该局部 PIE 结果，更新 T08 模板报告与阅读版，保留完整 TC03–TC05/G1 待验收。用户已将四项蓝图/重定向器资源纳入 `e44a44a`，LFS 属性与指针已核对；磁盘资产未检出 Cone/DirectionVisual，最终方向配置存盘仍需核对。助手仅本地提交文档，未创建分支、PR、推送或执行 UE。

CHG031（2026-10-11）：用户在 T09 PIE 收尾清单后确认“测试通过”，归档确认原文和当前可执行分支范围。补齐 Issue #11 最后一项验收及实际验证记录，关闭任务并同步 Project 为 Done；更新报告、任务/测试/技术设计、文档入口和阅读快照。此前 Editor 构建与 23 项原生 Automation 通过结果沿用，未重复执行 UE；正式胜负/重开继续由 T16 联调，完整 G1/G3 不随本次关闭通过。本轮文档仅本地提交，BP_MothPlayer 工作区改动保留给用户，推送及 PR 由用户操作。

CHG032（2026-10-11）：用户要求继续 T10；`5692d76` 接入单次爆炸、固定爆心、静态共同遮挡、Actor 去重与统一帧末推力，补齐死亡/禁用后的待消费 Launch 清理。UE commandlet 生成并保存 BP_DeviceBomb 引擎 Sphere 灰盒，参数 v0.12 新增 0.25s Spent 清理。Editor 构建及最终 32 项 Automation 全部 Success，九项新增含实际 World/运动和保存资产重新加载；初次编译/脚本修正记录保留。新增接入、验证报告和证据并同步阅读版，#12 为 Open / In Progress，图形 PIE、正式敌人、D01/完整连锁与 G2 待验收；只本地提交，用户资产改动保留，未推送或创建 PR。

CHG033（2026-10-11）：用户在 T10 当前 PIE 清单后确认“测试通过”，归档原文和用户报告的证据类型，补齐 #12 验收记录并关闭，Project 同步为 Done。更新接入、报告、任务/测试/技术设计、参数说明和阅读快照；正式敌人/掉落、实际 D01 组合、完整连锁和 G2 随后续任务验收。沿用 `5692d76` 的 Editor 构建和 32 项 Automation 结果，本轮未更改玩法或重复运行 UE；文档仅本地提交，用户自行推送及创建 PR。

CHG034（2026-10-11）：用户要求继续完成 T11；`a5903f6` 接入世界 -Z 承托探测、最终体积净空、ALaunchColumn 伸出/停留/收回、单次目标弹射、占用净空后实体支撑与关闭支撑时主动释放站立角色。DEC30 先记录灰盒方形截面、斜面最高接触、中心探地边缘与底部容差；参数 v0.13 补标签、1cm 容差和 1s Spent 反馈。保存 BP_DeviceLauncher / BP_LaunchColumn，引擎资源与 LFS 已核对；Editor 构建及最终 43 项 Automation 全 Success，11 项新增含真实运动/站立、D01/D02 顺序与保存资产加载。首轮 41/43、中间重载编译问题和修正日志保留。#13 为 Open / In Progress，当前真实角色 PIE、正式敌人/完整连锁和 G2 待验收；新增接入/模板报告/证据并同步阅读版，仅本地提交，用户资产改动保留，未推送或创建 PR。

CHG035（2026-10-11）：用户在 T11 探地碰撞排查后确认“通过 提交并pr”。归档用户确认与 15:19 PIE 的三次成功起柱/正常种子结束（含空中启动），保留此前 NoGround / UnsupportedSurface 和 ABP_MothMurdock 除零警告；接入明确 BlockAll 为 WorldStatic、BlockAllDynamic 为 WorldDynamic，不改变现有承托规则。#13 验收补齐，关闭并同步 Project Done；更新接入、报告、任务/测试/技术设计与阅读快照。沿用既有 Editor 构建与 43 项 Automation 结果，本轮仅文档及 GitHub 收尾，没有新增助手图形 PIE 执行。按本次用户授权提交、推送 develop 并向 main 创建包含 T10/T11 的 PR；用户已有 BP_MothPlayer 资产改动不纳入，正式敌人/掉落、完整连锁、独立包及 G2 仍待验收。

CHG036（2026-10-11）：用户要求继续推进 T12 / #14；`bc3ce46` 交付十项真实 World Tick / Chaos / P01 Sweep 连锁测试、布局阴性对照、可选 P01 spawn/impact/expired 日志，以及五组布局/14 件普通装置的 L_T12Chains 和创建脚本。DEC31 先约束只启动起点，不直接激活/命中后续目标或在连锁中传送对象；玩法参数 v0.13 不变。最终 Editor 构建和全量 53 项 Automation Success，首次 7/9、观察窗口修正、编译/脚本修正及最终结果分别归档。新增接入、报告与脱敏证据，更新任务/测试/技术设计/入口/阅读快照；#14 为 Open / In Progress，当前 Murdock 图形 PIE、正式敌人/完整 G2 仍待验收，仅本地提交，不推送到此前 T10/T11 PR #39，用户原有玩家资产改动保留。

CHG037（2026-10-11）：用户授权“可以 创建pr”，随后明确验收回复“全部通过，可以记录验收”。按用户确认归档 T12 五站连锁/布局对照、暂停恢复与结束清理；沿用此前 Editor 构建及全量 53 项 Automation，不新增助手图形 PIE 或录屏声明。#14 验收补齐并关闭，Project Done；更新接入、报告、设计状态、任务/测试/入口与阅读快照。T10/T11 PR #39 已合入 main，保持本地 develop，T12 独立推送远程 codex/t12-natural-chains → main 并按模板创建 [PR #40](https://github.com/YYchainsAw/MothEffect/pull/40)；只包含 T12 已提交内容，用户原有 BP_MothPlayer 改动保留。正式敌人/掉落、容量、整局重开、独立包和完整 G2 仍随后续任务验收。

CHG038（2026-10-11）：用户要求继续推进完成 T13 / #15。新增 World-local 容量登记、掉落总额预占、Active 最早启动安全替换、P01共享额度和即时释放、累计闲置/警告/Spent0.25s、显式/世界界外与拒绝旧世代/回调生成的重置清理；修正已Spent种子直接销毁时派生柱的最终清理。DEC32和参数v0.14记录运行时Config副本；新增十三项原生测试、Development控制台及接入/模板报告。初次C++编译完成但原DLL被当前编辑器占用而链接失败，不强制关闭或热重载；在Saved临时工程复制并核对相同源码、Content目录引用只读加载，完整隔离Editor构建通过，全量66项Automation Success（新增13/既有53），导出errors/warnings为零；125个源码文件和全部Config与当前工程逐字节核对一致，原工程DLL仍需关闭旧编辑器后完成正式链接。#15保持Open / In Progress，当前角色PIE、正式敌人/完整整局和G2待验收；只本地提交，不推送或创建PR，用户原有资产改动保留。

CHG039（2026-10-11）：用户确认已保存并关闭编辑器，继续完成T13原工程验证。首次重编译发现Unity合并测试的Floor辅助函数重名，62e6156改名LifecycleFloor；保留失败15.25s及旧DLL锁定历史。原工程Development Editor / Win64正式Unity编译/链接Succeeded（13.33s），新DLL在原工程UnrealEditor-Cmd / NullRHI进程中加载，于19:04:46香港时间完成全量66项Automation Success，新增13/既有53均通过，每项errors/warnings与失败/未运行计数为零。八项既有启动warning与构建弃用warning分别记录；玩法参数和用户玩家资产哈希未变。当前PIE尚未确认，#15仍Open / In Progress；同步接入、报告、设计/参数证据、进度和阅读快照，只本地提交，未推送或创建PR。

CHG040（2026-10-11）：用户在T13五项验收清单后确认“测试通过 创建pr”。按用户报告归档当前Murdock PIE的过期/累计持有放下投掷、暂停冻结、总容量回收保护、效果/弹丸结束与重复清理/重新PIE，结合原工程Editor正式构建及全量66项Automation，补齐#15验收并关闭、Project Done。快速七件Active、64发饱和/恢复与显式界外保留原生证据，不新增助手图形PIE或录屏。本轮只更新验收/参数证据/任务/设计/报告和阅读快照，不改玩法数值或重复运行UE。按本次授权向远程codex/t13-lifecycle推送并以main为base创建独立T13 PR，本地继续develop，用户BP_MothPlayer改动不纳入；正式敌人、完整整局/跨波、最终场地、独立包与完整G2继续随后续任务验收。
