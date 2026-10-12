# Moth Effect（飞蛾效应）：开发文档入口

版本 v0.40 · 2026-10-12 · 单人 UE 5.8 C++ 与蓝图混合开发

2026-10-12 T16/T17 PR记录：按用户要求“创建pr”“保持待校验”，已推送codex/t16-t17-wave-arena → main并创建[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45)。main尚无T16，因此同一PR交付三波/胜负/重开及T17主竞技场，共51件功能/证据改动、11件新LFS资产；使用Refs #18/#19，两项Issue继续Open / In Progress，四组图形PIE分别待校验、完整验收框未勾选。原工程T16最终95项Editor/5项Game及T17主场地8项Game沿用既有真实记录，本次仅PR与文档，不重复执行UE。独立PR工作树验证271件文件/49件UE资产、42节阅读版、diff及LFS fsck通过；11件LFS对象220KB已上传，用户玩家蓝图未纳入。本地继续develop，GitHub Repository checks按最终head另行核对，静态CI不代替游戏验收。以下段落保留历史。

2026-10-12 合并与T17推进：PR #42/#43/#44已合并到main，本地develop以cbe4330同步origin/main 0780180，保留用户玩家蓝图。L_Arena_01已落地60×50米三区、六低/三高掩体、双侧120cm平台坡道及南北回转通路；八点出生沿用T16，默认启动改为主竞技场，Ready由MothStartEncounter开始，完整UI继续T18。八件新LFS资产保存重载后15条路径完整，原工程Editor构建27.15s Succeeded，真实主地图8项Game全部Success，errors=0 / warnings=156，新增三项场地测试无告警；既有Paragon/Murdock资源告警保留。实现d9b3851、测试dce5cc6已本地提交，T17 #19 Open / In Progress，四组图形PIE待验收，战斗参数v0.17未变。本地T16依赖仍未推送/创建PR；此次合并不扩大先前手动验收范围。以下段落保留历史。 入口见[Arena_Setup](05_Development_Guide/Arena_Setup.md)，实际证据见[T17报告](06_Test_Doc/T17_Report_2026-10-12.md)。

2026-10-12 T16推进：三波导演已接入4 / 3+3 / 4+4固定种类/出生点，准备与延迟批次均计入待生成；真实胶囊/静态导航/安全距离检查和完整预警，占位后只在原点重试。玩家死亡/界外判负，末波结束胜利，World-local开关先关闭伤害/外力/生成再清对象与任务/绑定；当前地图OpenLevel重开，暂停结果页可重开，跨波不刷新Dormant闲置。保存BP_WaveDirector、原创警戒材质和L_T16Flow三件资产及八点静态导航，参数v0.17/Config和DEC35同步。Editor构建、最终全量95项Editor、5项真实Game流程与2项Game导航Success；实际4/6/8出生及8存活峰值、批次空窗、物理预警中断、界外、跨波闲置和十次全新World重开有证据。最终Editor导出errors=0 / warnings=1（引擎联网探测超时），此前94项warnings=0；Game流程errors=0，既有Paragon/Murdock加载/动画等156条warnings如实留存。#18保持Open / In Progress，当前图形PIE待验收；T14/T15仍待验收，T17最终场地、T18完整UI/教程及独立包/G3继续对应任务。本轮develop本地提交7d4edba/8932848/dca4554，未推送/创建T16 PR，用户玩家资产未纳入。以下段落保留历史。 入口见[Wave_Flow_Setup](05_Development_Guide/Wave_Flow_Setup.md)，证据见[T16报告](06_Test_Doc/T16_Report_2026-10-12.md)。

2026-10-12 T15 PR记录：用户要求“先保持待验收 创建pr”，已推送codex/t15-enemy-drops并创建[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43)，当前Open。T14 PR #42尚未合并，T15暂以codex/t14-shooting-ai为目标，只显示T15的36件改动，待#42合并后再改为main。#17继续Open / In Progress，当前图形PIE待验收、验收框保留未勾选；不关闭任务。原工程Editor构建、88项全量与最终11项复测/2项Game导航沿用已归档实测，本次仅PR与文档记录，未重复运行UE。本地继续develop，玩家原有资产改动未纳入。现有workflow仅自动检查main/develop目标，叠加PR按最终head手动运行Repository checks，实际结果另以GitHub检查页为准。以下段落保留历史。

2026-10-12 T15推进：三个BP_EnemyBase子类以青色三角棱柱D01、橙球D02、紫长条D03预告固定掉落，共用T14射击AI；生命死亡后只尝试一次，近处有效出生、真实物理Dormant、24额度回收/保护与回调世代取消统一接入。保存12件新资产与L_T15Drops静态导航，参数v0.16/Config和DEC34同步。原工程Editor构建通过，全量88项Editor Success（新增11/既有77）；仅测试夹具修正后新增11项再次Success、2项Game保存导航Success，导出errors/warnings=0。T15 #17保持Open / In Progress、当前Murdock图形PIE待验收；T14 #16仍待用户PIE，PR #42未追加本轮内容。本轮develop本地提交，无T15推送/PR。以下段落保留历史。 入口见[Enemy_Drops_Setup](05_Development_Guide/Enemy_Drops_Setup.md)，证据见[T15报告](06_Test_Doc/T15_Report_2026-10-12.md)。

2026-10-12 T14 PR记录：按用户要求已推送远程codex/t14-shooting-ai → main并创建[T14 PR #42](https://github.com/YYchainsAw/MothEffect/pull/42)，当前Open；T13 PR #41已合并。用户明确确认“尚未完成，保留待验收”，当前Murdock图形PIE不记通过，#16保持Open / In Progress；合并后仍未验收则转Pending Acceptance。原工程Editor构建、77项Editor与2项Game导航沿用2026-10-11源码678dc66的实际证据；首个PR head为1a18890，Repository checks已Success（run38189638920），本次仅同步交付文档与阅读版，最终head检查另按GitHub实际结果核对。本地继续develop，玩家原有蓝图改动保留。以下段落保留历史。

2026-10-11 T14推进：EnemyBase一套射击AI已接入视线/完整预警/共用P01、最后可见位置追踪、静态导航与停滞持续换点、共享生命/推力及幂等死亡。参数v0.15与Config同步，保存BP_EnemyBase和带掩体/平台/已构建NavMesh的L_T14AI。原工程Editor构建通过，全量77项Editor Automation Success（新增11/既有66），另一个实际加载保存场地的Game进程2项导航Success；当前Murdock图形PIE待验收，#16为Open / In Progress。入口见[Enemy_Setup](05_Development_Guide/Enemy_Setup.md)，范围见[T14_Report](06_Test_Doc/T14_Report_2026-10-11.md)。本轮按用户要求逐功能本地提交，未推送到T13 PR #41；玩家原有资产改动保留。以下段落保留历史。

2026-10-11 T13 验收完成：统一 World-local 容量、累计 Dormant 闲置时钟、过期提示与 Spent/界外/重置清理已接入，基线 v0.14 保留 24/6/6/64 与 30s/5s，新增过期 Spent 反馈 0.25s。用户保存并关闭编辑器后，修正 Unity 合并编译的测试辅助函数重名，原工程 Editor 构建/链接成功；使用原工程新 DLL 完成全量66项Automation Success；用户在五项验收清单后回复“测试通过 创建pr”，当前真实角色PIE按用户确认归档，#15为Closed / Done。完整整局、正式敌人和G2继续随后续任务验收。入口见 [Lifecycle_Setup](05_Development_Guide/Lifecycle_Setup.md)，范围见 [T13_Report](06_Test_Doc/T13_Report_2026-10-11.md)。实现f9153fd与Unity修正62e6156已本地提交；按本次授权已推送远程codex/t13-lifecycle → main并创建[T13 PR #41](https://github.com/YYchainsAw/MothEffect/pull/41)，当前Open，本地继续develop，用户玩家资产改动保留。以下 T12 段落保留历史。

2026-10-11 T12 完成：`bc3ce46` 已交付十项真实 World Tick 连锁测试、P01 可选开发日志与五组布局的 L_T12Chains；最终 Editor 构建与全量 53 项 Automation Success，参数 v0.13 不变。用户明确回复“全部通过，可以记录验收”，据此归档 Direct / BombHit / BombMiss / ColumnHit / ColumnMiss 五站及暂停恢复、结束清理的当前角色 PIE 确认，#14 为 Closed / Done。入口见 [Natural_Chain_Setup](05_Development_Guide/Natural_Chain_Setup.md)，证据和边界见 [T12_Report](06_Test_Doc/T12_Report_2026-10-11.md)。正式敌人/掉落、容量、完整整局重开、独立包与完整 G2 继续随后续任务验收。用户授权后已创建 [T12 PR #40](https://github.com/YYchainsAw/MothEffect/pull/40)，codex/t12-natural-chains → main；依赖的 T10/T11 PR #39 已合入 main，本地继续 develop。以下 T11 段落保留历史。

2026-10-11 T11 完成：`a5903f6` 已交付 D01 静态承托探测、最终体积净空、单次弹射、净空后实体支撑和安全收回；BP_DeviceLauncher / BP_LaunchColumn 已保存，Editor 构建与全量 43 项 Automation 通过。用户在地面碰撞排查后确认“通过 提交并pr”，据此归档当前 PIE 验收，Issue #13 为 Closed / Done。15:19 最新 PIE 日志另有三次成功起柱与种子正常结束，其中一次从 Z=796.664cm 空中启动；阶段、逐目标弹射及站立手感按用户确认记录，不由生成日志单独推断。参数 v0.13；入口见 [Launcher_Setup](05_Development_Guide/Launcher_Setup.md)，证据与报告见 [T11_Report](06_Test_Doc/T11_Report_2026-10-11.md)。本轮按用户授权提交、推送并从 develop 向 main 创建包含 T10/T11 的 PR；正式敌人、完整连锁和 G2 继续随对应任务验收。

2026-10-11 T10 完成：D02 单次爆炸、固定爆心、Actor 去重、静态遮挡伤害/推力与统一帧末外力结算已接入，BP_DeviceBomb 灰盒已生成并通过重新加载验证。Editor 构建和全量 32 项 Automation 通过，新增九项；参数 v0.12 新增 Spent 清理 0.25s。用户在接入清单后确认“测试通过”，当前 PIE 验收已归档，Issue #12 为 Closed / Done。正式敌人、D01 组合、完整连锁与 G2 随后续任务验收；下一项为 T11。入口见 [Bomb_Setup](05_Development_Guide/Bomb_Setup.md)，证据与 PR 报告见 [T10_Report](06_Test_Doc/T10_Report_2026-10-11.md)。

2026-10-11 T09 完成：本机 UE 5.8 Editor 增量构建成功；九项玩家边界测试复现并修正无 Flush 应用失焦续射、UnPossessed 后旧角色续射，修正后 MothEffect 共 23 项 Automation 全部 Success，未预期 errors/warnings 为零。用户随后确认当前真实角色 PIE“测试通过”，确认原文与通过范围已归档；Issue #11 验收和验证记录已补齐，状态为 Closed / Done。实际胜负/整局重开随 T16 补测，G1/G3 继续按完整证据验收。验证范围和 PR 报告见 [T09 报告](06_Test_Doc/T09_Report_2026-10-11.md)。源码和文档仅本地提交，用户自行推送和创建 PR。

2026-10-08 玩家迁移：Murdock 模型和配套动画、一体枪口、单 Jog 移动的现行接入见 [Murdock_Setup](05_Development_Guide/Murdock_Setup.md)。参数 v0.11 与新 JogSpeed 默认值同步，助手未编译/运行 UE；旧模板动画与独立枪步骤保留为历史或可选模式。

本目录按用户建立的开发阶段结构归档。文件使用稳定的英文语义名称，正文为中文；v0.3 整理结构，v0.4 配置版本管理，v0.5 清理个人路径与发布配置，v0.6 将 Content 的 Git 范围收敛为 MothEffect 子目录，v0.7 统一英文名 Moth Effect 与中文名飞蛾效应，v0.8 新增五种待选机关，v0.9 确认玩家方案，v0.10 开始 T04 C++，v0.11 补充动画接线，v0.12 开始 T05，v0.13 改为角色组件内预览和调整武器，v0.14 同步 GitHub 开发流程、六阶段排期与任务状态。v0.15 核对本机/远程的实际实现，确认每日 4–6 小时，并预留少量原创机关/场景资产和 UI；v0.16 记录 T05 完成并接入 T06 原生机关状态与命中基础。各文件保留自身最近更新版本；文档源文件是 Markdown/JSON，[浏览器阅读版](01_Project_Overview/Reading_View.html) 是汇总快照。

工程：[MothEffect.uproject](../MothEffect.uproject) · 英文名：Moth Effect · 中文名：飞蛾效应 · 风格：魔法朋克。GitHub 仓库已公开并存在提交，`main`/`develop` 已纳入轻量 Actions 和 Issue/PR 模板；Project 与 M1–M6 已建立。日常使用见 [GitHub 开发流程](05_Development_Guide/GitHub_Workflow.md)，提交范围与资源依赖见 [版本管理](03_Code_Standard/Version_Control.md)。

v0.18 接入 T08：D03 世界方向锁定/定时发射与 P01 统一弹道源代码已交付，用户需完整编译、创建可见灰盒 BP、运行 12 项 Automation 并记录 TC03–TC05；入口见 [Emitter_Setup](05_Development_Guide/Emitter_Setup.md)，报告见 [T08_Report](06_Test_Doc/T08_Report_2026-10-08.md)。本轮没有 UE 玩法通过结果；T07 仅基础拾取、放下和投掷通过，其余边界继续待验收。用户自本轮授权助手每个小功能完成后本地提交，继续沿用 develop，用户自行推送和操作 PR。

v0.19 归档 T08 局部结果：Devices 九项 Automation Success，含三个 Emitter 案例和既有六项重测；三处 World 收尾警告已在 `23eb829` 修正，修正后复测、P01 三项与 T08 PIE 仍待执行。原始结果见 [T08 验证日志](06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)，历史源码交付记录不覆盖最新通过范围。

v0.20 最新复测：2026-10-08 17:06 的十二项 Automation 全部 Success，包含 Devices 九项和 Projectiles 三项，World 清理警告没有再出现；证据见 [T08 验证记录](06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。下一步按 [Emitter_Setup 第 6 节](05_Development_Guide/Emitter_Setup.md#6-pie-具体验收) 验证地面/空中方向及十次投掷成功率，G1 未验收。

玩家方案已由用户确认：首版不使用 GAS，复用本地 Rifle 动画，采用 C++ 玩家行动状态机、AnimBP 移动状态机与上半身 Montage。开发入口为 [技术设计第 3 节](02_Design_Doc/TDD/Technical_Design.md#3-越肩射击与输入)，依据见 DEC22–DEC27；操作规则为 R19–R23，验收为 TC34–TC38。此前核对 T01–T03 为 Closed / Done，T04 为 Closed / Pending Acceptance，T05 为 Closed / Done、[PR #34](https://github.com/YYchainsAw/MothEffect/pull/34) 已合并。2026-10-08 归档 T06 当前三项 Automation 与 PIE 分支通过，记录见 [Device_Setup](05_Development_Guide/Device_Setup.md)，等待用户提交记录并走 PR；GitHub 状态本次未修改。完整案例及 G0–G4 以真实证据验收，详见 [开发计划](05_Development_Guide/Development_Plan.md)；[10/7 进度核查](05_Development_Guide/Progress_Audit_2026-10-07.md) 保留当次历史快照。UI 预留 4–6 小时、少量原创资产预留 6–8 小时，必要反馈另留 2–3 小时；修订节点保持 10/17 DDL。

武器预览接入：BP_ThirdPersonCharacter 的 Mesh 下使用 RifleComponent，由子 Actor 组件创建完整 BP_Rifle，在角色视口直接调整组件 Transform；Rifle Class 与 Rifle Attach Socket 保留在角色默认值。T05 Issue 已勾选预览与运行仅一把武器，具体构建与证据仍待补；旧偏移迁移及检查步骤见 [Weapon_Setup](05_Development_Guide/Weapon_Setup.md)。

## 1. 目录与有效文件

| 目录 | 文件 | 职责 |
|---|---|---|
| 01_Project_Overview/ | [Project_Overview.md](01_Project_Overview/Project_Overview.md) | 项目概览 |
| 01_Project_Overview/ | [Reading_View.html](01_Project_Overview/Reading_View.html) | 浏览器阅读版 |
| 02_Design_Doc/GDD/ | [Game_Design.md](02_Design_Doc/GDD/Game_Design.md) | 游戏策划案 |
| 02_Design_Doc/GDD/ | [Device_Interaction_Rules.md](02_Design_Doc/GDD/Device_Interaction_Rules.md) | 道具与交互规则 |
| 02_Design_Doc/GDD/ | [Device_Candidates.md](02_Design_Doc/GDD/Device_Candidates.md) | 候选机关的效果、组合与待验证边界；未进入首版排期 |
| 02_Design_Doc/GDD/ | [Level_UI_Asset_Specification.md](02_Design_Doc/GDD/Level_UI_Asset_Specification.md) | 关卡、界面与资源规格 |
| 02_Design_Doc/GDD/ | [Gameplay_Parameters.json](02_Design_Doc/GDD/Gameplay_Parameters.json) | 玩法参数基线 |
| 02_Design_Doc/TDD/ | [Technical_Design.md](02_Design_Doc/TDD/Technical_Design.md) | 技术设计，含玩家状态/输入/动画/GAS 决策与实施顺序 |
| 02_Design_Doc/TDD/Decisions/ | [Design_Decisions.md](02_Design_Doc/TDD/Decisions/Design_Decisions.md) | 设计决策 |
| 03_Code_Standard/ | [Coding_Conventions.md](03_Code_Standard/Coding_Conventions.md) | 代码与资产命名规范 |
| 03_Code_Standard/ | [Version_Control.md](03_Code_Standard/Version_Control.md) | 版本管理规范 |
| 04_Engine_Config/ | [UE_Project_Configuration.md](04_Engine_Config/UE_Project_Configuration.md) | UE 工程配置 |
| 05_Development_Guide/ | [Development_Plan.md](05_Development_Guide/Development_Plan.md) | 开发任务与排期 |
| 05_Development_Guide/ | [Progress_Audit_2026-10-07.md](05_Development_Guide/Progress_Audit_2026-10-07.md) | 27 条任务的静态实现/资产审计、可复用前置与缺失证据 |
| 05_Development_Guide/ | [Player_Setup.md](05_Development_Guide/Player_Setup.md) | 玩家 C++ 交付范围、输入引用、ABP 数据/移动/瞄准接线与手动验收 |
| 05_Development_Guide/ | [Murdock_Setup.md](05_Development_Guide/Murdock_Setup.md) | Murdock 官方 ABP 复用、单 Jog、一体枪口、开火/换弹及交互迁移与待执行验证 |
| 05_Development_Guide/ | [Weapon_Setup.md](05_Development_Guide/Weapon_Setup.md) | T05 步枪/输入/测试靶/上半身动画配置与分支验收 |
| 05_Development_Guide/ | [Device_Setup.md](05_Development_Guide/Device_Setup.md) | T06 原生机关状态、灰盒资产、重复命中与当前验收步骤 |
| 05_Development_Guide/ | [Device_Interaction_Setup.md](05_Development_Guide/Device_Interaction_Setup.md) | T07 挂点、持物表现、安全释放、输入与已通过范围 |
| 05_Development_Guide/ | [Emitter_Setup.md](05_Development_Guide/Emitter_Setup.md) | T08 D03/P01 灰盒 BP、方向提示、Automation 和空中启动验收 |
| 05_Development_Guide/ | [Bomb_Setup.md](05_Development_Guide/Bomb_Setup.md) | T10 爆炸灰盒、统一推力与当前 PIE 验收 |
| 05_Development_Guide/ | [Launcher_Setup.md](05_Development_Guide/Launcher_Setup.md) | T11 探地标签、升柱灰盒、安全支撑与当前 PIE 验收 |
| 05_Development_Guide/ | [Natural_Chain_Setup.md](05_Development_Guide/Natural_Chain_Setup.md) | T12 普通机关测试房、真实连锁与布局对照 PIE 清单 |
| 05_Development_Guide/ | [Lifecycle_Setup.md](05_Development_Guide/Lifecycle_Setup.md) | T13 容量、累计闲置、过期与清理接入和 PIE 清单 |
| 05_Development_Guide/ | [Enemy_Setup.md](05_Development_Guide/Enemy_Setup.md) | T14射击AI、已保存导航场景与当前PIE清单 |
| 05_Development_Guide/ | [Enemy_Drops_Setup.md](05_Development_Guide/Enemy_Drops_Setup.md) | T15色形预告、固定掉落与四项PIE清单 |
| 05_Development_Guide/ | [Wave_Flow_Setup.md](05_Development_Guide/Wave_Flow_Setup.md) | T16三波、胜负/重开接口与四项PIE清单 |
| 05_Development_Guide/ | [Arena_Setup.md](05_Development_Guide/Arena_Setup.md) | T17主竞技场、保存导航及四组图形PIE清单 |
| 05_Development_Guide/ | [GitHub_Workflow.md](05_Development_Guide/GitHub_Workflow.md) | Issue/PR 模板、Project 视图、分支与轻量 Actions 的实际用法 |
| 05_Development_Guide/ | [Documentation_Workflow.md](05_Development_Guide/Documentation_Workflow.md) | 文档维护与变更流程 |
| 05_Development_Guide/ | [Changelog.md](05_Development_Guide/Changelog.md) | 变更日志 |
| 06_Test_Doc/ | [Test_Plan.md](06_Test_Doc/Test_Plan.md) | 测试计划与验收 |
| 06_Test_Doc/ | [T09_Report_2026-10-11.md](06_Test_Doc/T09_Report_2026-10-11.md) | T09 修复、原生验证证据、PIE 收尾步骤与 PR 报告 |
| 06_Test_Doc/ | [T10_Report_2026-10-11.md](06_Test_Doc/T10_Report_2026-10-11.md) | T10 爆炸/推力交付、资产及原生验证范围与 PR 报告 |
| 06_Test_Doc/ | [T11_Report_2026-10-11.md](06_Test_Doc/T11_Report_2026-10-11.md) | T11 升柱/弹射交付、原生与资产验证范围及 PR 报告 |
| 06_Test_Doc/ | [T12_Report_2026-10-11.md](06_Test_Doc/T12_Report_2026-10-11.md) | T12 真实连锁、保存场景/资产验证范围与首次失败记录 |
| 06_Test_Doc/ | [T13_Report_2026-10-11.md](06_Test_Doc/T13_Report_2026-10-11.md) | T13 容量/闲置/清理、构建环境与验证范围 |
| 06_Test_Doc/ | [T14_Report_2026-10-11.md](06_Test_Doc/T14_Report_2026-10-11.md) | T14原生/导航验证、首轮失败、资产与验收边界 |
| 06_Test_Doc/ | [T15_Report_2026-10-12.md](06_Test_Doc/T15_Report_2026-10-12.md) | T15真实击杀/落地/容量验证与待验收记录 |
| 06_Test_Doc/ | [T16_Report_2026-10-12.md](06_Test_Doc/T16_Report_2026-10-12.md) | T16实际三波/连续重开验证、失败及既有告警边界 |
| 06_Test_Doc/ | [T17_Report_2026-10-12.md](06_Test_Doc/T17_Report_2026-10-12.md) | T17坡道/P01/导航与主场地8项Game结果，图形PIE待验收 |
| 07_Release/ | [Release_Checklist.md](07_Release/Release_Checklist.md) | 发布检查清单 |
| Assets/UML/ | 当前为空 | 后续实际产出的 UML 图及源文件 |

GDD = 游戏设计文档，TDD = 技术设计文档。玩法数值归属 GDD，UE 配置归属 Engine_Config。决策依据与变更历史分开维护；测试与发布也各自独立。

## 2. 按开发流程阅读

1. **目标与范围**：[项目概览](01_Project_Overview/Project_Overview.md) → [游戏策划案](02_Design_Doc/GDD/Game_Design.md)，明确本次要交付的体验和 P0/P1/P2。
2. **行为与内容**：[道具与交互规则](02_Design_Doc/GDD/Device_Interaction_Rules.md) + [玩法参数基线](02_Design_Doc/GDD/Gameplay_Parameters.json) + [关卡、界面与资源规格](02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，明确状态、边界、波次与资源。
3. **技术与环境**：[技术设计](02_Design_Doc/TDD/Technical_Design.md) + [代码与资产命名规范](03_Code_Standard/Coding_Conventions.md) + [版本管理规范](03_Code_Standard/Version_Control.md) + [UE 工程配置](04_Engine_Config/UE_Project_Configuration.md)。
4. **执行与验证**：按 [开发任务与排期](05_Development_Guide/Development_Plan.md) 的 G0–G4 推进，用 [测试计划与验收](06_Test_Doc/Test_Plan.md) 留下实际证据。
5. **变更与发布**：改动遵循 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)，更新 [设计决策](02_Design_Doc/TDD/Decisions/Design_Decisions.md) 与 [变更日志](05_Development_Guide/Changelog.md)，提交时执行 [发布检查清单](07_Release/Release_Checklist.md)。

## 3. 维护约定

目录阶段编号保持现状；文件名用英文语义名称和下划线，不重复加另一套顺序编号。REQ/R/D/T/G/TC/DEC/CHG/RISK 等 ID 不随文件迁移改变。

交互、接口、内容、数值各保留唯一来源，具体分工见 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)。参数账本不是可直接导入 UE 的 DataTable；此前只改代码的移动速度差异仍按 Player_Setup 记录。T08 新增弹丸几何半径、出生净距和 Spent 视觉残留默认值，在参数 v0.11 中标记为未实测，既有速度/伤害/发射节奏不变。当前任务开关状态、Project Status 与验收证据分别记录，进度快照和 Issue 对照在开发计划维护。

修改源文件后同步更新版本、相对链接和阅读快照；不要只改 HTML。原始平铺文件已在工程外备份，避免在 Docs 中并存两套有效文件。
