# Moth Effect（飞蛾效应）：开发任务与排期

版本：v0.49｜更新日期：2026-10-14｜时间：香港时间（UTC+8）

2026-10-14 当前工作为用户确认的道具基础重构，在当前 `refactor` 分支分段本地提交。`f6d6b2a` 接入地面固定/尺寸、D03 水平投影与共用激活延迟，`f91db81` 接入爆炸激活和固定设备的显式外力。新规则、参数 v0.19、DEC41–DEC44 及 [重构接入](Device_Refactor_Setup.md) 已同步；收尾提交 0e5370a / 4934627，隔离 UE 构建、114 Editor 与 12 主地图 Game 回归通过；当前硬件 PIE / 手感及独立包待验收，见 [验证报告](../06_Test_Doc/Device_Refactor_Report_2026-10-14.md)。GitHub 四项均 Open / In Progress，负责人 YYchainsAw、M5 · 试玩与候选包验收和现有标签均已回读；不以本地代码提交关闭待验收任务。

原工程补充验证：确认编辑器关闭后，Editor 常规构建 56.79 秒 Succeeded，新 DLL 的六项 Foundation 全通过；重新打开项目即可加载。该构建包含当前用户未提交控制器代码，未将这些改动纳入本轮提交。

| 本轮工作 | Issue | 已实现范围与待验收目标 | 当前状态 |
|---|---|---|---|
| 地面道具固定与尺寸 | [#48](https://github.com/YYchainsAw/MothEffect/issues/48) | 地面根停止模拟/NoCollision，ShotCollider 保留查询；VisualScale 与 Body/Shot 半径独立可调；显式推力/弹射起飞，再落地固定，支持移除恢复下落；掉落/拾取/释放/容量回归 | 原生回归通过；硬件 PIE 待验收 |
| D03 水平射向 | [#49](https://github.com/YYchainsAw/MothEffect/issues/49) | 入射 XY 投影归一化、Z=0，合法竖直命中回退世界 +X；方向显示、四向/斜向/空中与真实 P01 联调 | 原生回归通过；硬件 PIE 待验收 |
| 三机关共用激活延迟 | [#50](https://github.com/YYchainsAw/MothEffect/issues/50) | 接纳即 Active/占容量，统一延迟后效果；等待期幂等、暂停冻结、结束/销毁/重开取消；D03 工作寿命与首发延迟从效果起动计 | 原生回归通过；硬件 PIE 待验收 |
| 爆炸激活连锁 | [#51](https://github.com/YYchainsAw/MothEffect/issues/51) | 无遮挡 Dormant 经 TryActivate 等待自身延迟；Held/Active 保护，下游独立 Guid、各炸弹伤害/外力去重；遮挡、回路、零延迟和清理回归 | 原生回归通过；硬件 PIE 待验收 |

新工作不重排 T01–T28，不把旧规则专项的通过结论套到新规则。下方 T08/T10/T12 等任务与证据保留其原版本历史；当前“地面持续物理、三维方向、立即效果、爆炸不启动”的行为已由以上四项取代。阅读快照与本轮实际测试随后记录。

2026-10-13 已按用户要求创建[PR #47](https://github.com/YYchainsAw/MothEffect/pull/47)：T18编辑入口、T19默认反馈与T20可替换原型外观，codex/t18-t20-prototype-feedback → main。base ce3a887、首个head 3a7d837，完整Git tree与已验证develop cadff96一致；126件差异，46件LFS对象11MB已上传，用户玩家资产未纳入。Assignee YYchainsAw，Labels feature/polish/area:ui/area:combat/area:art，Project MothEffect · 首版开发 / In Progress，Milestone M5 · 试玩与候选包验收，均已回读核对。首个head的Repository checks已Success（6s）；静态CI不代替游戏验收。使用Refs #20/#21/#22，关联Issue保持Open，各轮手动验收保留。本地继续develop；本轮PR沿用既有真实UE证据，随后T21独立包另行记录。 以下段落保留历史。

2026-10-13 T20原型完成：三种原创机关外壳、金属掩体模块、符文灯饰、共用材质与DA_PrototypeArt共17资产保存并新进程重载，重复运行文件SHA保持一致。机关与敌人背包共用可替换外观，主场地九块掩体与三灯饰由可关闭场景层接入；现有碰撞/导航和v0.18玩法基线保留。构建、107项全量Editor回归及最后一项夹具修正后的3项专项、实际D3D12/音频主地图12项通过；首轮围墙误识别、Static夹具、旧灰盒断言与导入失败如实留档。用户后续深度修改机制与整套UI，当前实现作为可运行基线；五组真实玩家/场地/独立包待验收，第三方缺依赖与完整恢复待补。实现3c6c8e5本地develop提交，#22 Open / In Progress，未推送/创建PR；配置见[Prototype_Art_Setup](Prototype_Art_Setup.md)，结果见[T20报告](../06_Test_Doc/T20_Report_2026-10-13.md)。以下段落保留历史。

2026-10-13 T19默认反馈完成：默认材质/配置/空间衰减与8个原创短音效，共11资产保存并新进程重载；真实提交后的开火/命中/伤害/三机关启动与过期反馈接入，D03世界锁定箭头及色形状态由原生组件继承，Designer保持用户制作入口。方向图形按世界向量构造，反馈出生回调结算/清理不得回生，缺失HUD字体来源只补来源不改样式。构建、104项Editor回归及最终9项反馈/UI、真实D3D12/音频主地图11项通过，首次夹具失败与五张原始截图见[T19报告](../06_Test_Doc/T19_Report_2026-10-13.md)。#21保持Open，Project In Progress，四组当前PIE/听感待验收；实现8112648与修正e00348f本地develop提交，未推送/创建PR。调节入口见[Feedback_Setup](Feedback_Setup.md)。以下段落保留历史。

2026-10-13 T18编辑接入：PR #46已合并main，develop已同步。保存Designer基础控件树，补25个必需绑定、四按钮生命周期、Class Defaults菜单文案/预览与教学配置Data Asset；用户明确具体UI由自己制作。Editor构建、2项编辑回归、主地图10项与扩充UI2项通过，告警/失败记录见[T18报告](../06_Test_Doc/T18_Report_2026-10-12.md)。本轮本地提交，未推送/创建新PR；#20保持Open，五组硬件PIE与独立包待验收。制作入口见[UI_Setup](UI_Setup.md)。以下段落保留历史。

2026-10-12 T18 PR记录：按用户要求已推送 codex/t18-hud-tutorial → main 并创建 [PR #46](https://github.com/YYchainsAw/MothEffect/pull/46)。创建期间核对到 T16/T17 PR #45 已合并至 main 8481463，因此 T18 直接面向 main；合并本身不扩大前轮手动验收范围。PR 的 Assignee 为 YYchainsAw，Labels 为 feature / area:ui，Project 为 MothEffect · 首版开发（Status In Progress），Milestone 为 M4 · 三波可玩闭环，均继承 #20 并在 GitHub 回读核对。使用 Refs #20，Issue 继续 Open / In Progress、六项完整验收框未勾选、五组当前硬件 PIE 待验收。实现阶段96项 Editor/主地图10项 Game 和最终三分辨率 UI 沿用已归档证据，本次 PR/元数据/文档未重复 UE；独立 PR 工作树297文件/50UE资产、44节阅读版、diff与LFS fsck通过。原开发与PR初始交付的完整 Git tree 相同，用户玩家蓝图未纳入。本地继续 develop；最终 head 的 Repository checks 另按实际 GitHub 运行核对。 以下段落保留历史。

2026-10-12 T18推进：原生 UMG HUD、三种原创矢量图标、实际动作驱动的三步教学和标题/暂停/教学完成/胜负/重开面板已接入。首次读取真实生命/弹药/持物/波次，事件更新并在换玩家/重开时解绑；进入战斗重载关卡，仅保存一次开始意图。教学分别核验爆炸死亡来源、真实物理抬升、投出且空中 W01 启动、D03 P01→D02 间接击杀；落地/错误启动补充，独立目标允许换顺序，暂停保留合法换弹。实现阶段96项 Editor 全量及主地图10项 Game 回归通过，后续教学重试/最终布局的 UI 分支在三种分辨率真实 D3D12 离屏渲染复测通过；既有资源/动画告警保留。#20 Open / In Progress，五组当前硬件输入与完整图形 PIE 待验收，TC26 外部盲玩及独立包留后续；本轮本地 develop 提交，未推送/创建 T18 PR，T16/T17 PR #45 待校验状态保留。 接入与五组清单见 [UI_Setup](UI_Setup.md)，结果见 [T18报告](../06_Test_Doc/T18_Report_2026-10-12.md)。 以下段落保留历史。

2026-10-12 T16/T17 PR记录：按用户要求“创建pr”“保持待校验”，已推送codex/t16-t17-wave-arena → main并创建[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45)。main尚无T16，因此同一PR交付三波/胜负/重开及T17主竞技场，共51件功能/证据改动、11件新LFS资产；使用Refs #18/#19，两项Issue继续Open / In Progress，四组图形PIE分别待校验、完整验收框未勾选。原工程T16最终95项Editor/5项Game及T17主场地8项Game沿用既有真实记录，本次仅PR与文档，不重复执行UE。独立PR工作树验证271件文件/49件UE资产、42节阅读版、diff及LFS fsck通过；11件LFS对象220KB已上传，用户玩家蓝图未纳入。本地继续develop，GitHub Repository checks按最终head另行核对，静态CI不代替游戏验收。以下段落保留历史。

2026-10-12 合并与T17推进：PR #42/#43/#44已合并到main，本地develop以cbe4330同步origin/main 0780180，保留用户玩家蓝图。L_Arena_01已落地60×50米三区、六低/三高掩体、双侧120cm平台坡道及南北回转通路；八点出生沿用T16，默认启动改为主竞技场，Ready由MothStartEncounter开始，完整UI继续T18。八件新LFS资产保存重载后15条路径完整，原工程Editor构建27.15s Succeeded，真实主地图8项Game全部Success，errors=0 / warnings=156，新增三项场地测试无告警；既有Paragon/Murdock资源告警保留。实现d9b3851、测试dce5cc6已本地提交，T17 #19 Open / In Progress，四组图形PIE待验收，战斗参数v0.17未变。本地T16依赖仍未推送/创建PR；此次合并不扩大先前手动验收范围。以下段落保留历史。 四组待验收见[Arena_Setup](Arena_Setup.md)，结果见[T17报告](../06_Test_Doc/T17_Report_2026-10-12.md)。

2026-10-12 T16推进：三波导演已接入4 / 3+3 / 4+4固定种类/出生点，准备与延迟批次均计入待生成；真实胶囊/静态导航/安全距离检查和完整预警，占位后只在原点重试。玩家死亡/界外判负，末波结束胜利，World-local开关先关闭伤害/外力/生成再清对象与任务/绑定；当前地图OpenLevel重开，暂停结果页可重开，跨波不刷新Dormant闲置。保存BP_WaveDirector、原创警戒材质和L_T16Flow三件资产及八点静态导航，参数v0.17/Config和DEC35同步。Editor构建、最终全量95项Editor、5项真实Game流程与2项Game导航Success；实际4/6/8出生及8存活峰值、批次空窗、物理预警中断、界外、跨波闲置和十次全新World重开有证据。最终Editor导出errors=0 / warnings=1（引擎联网探测超时），此前94项warnings=0；Game流程errors=0，既有Paragon/Murdock加载/动画等156条warnings如实留存。#18保持Open / In Progress，当前图形PIE待验收；T14/T15仍待验收，T17最终场地、T18完整UI/教程及独立包/G3继续对应任务。本轮develop本地提交7d4edba/8932848/dca4554，未推送/创建T16 PR，用户玩家资产未纳入。以下段落保留历史。 接入与四项PIE清单见[Wave_Flow_Setup](Wave_Flow_Setup.md)，报告见[T16_Report](../06_Test_Doc/T16_Report_2026-10-12.md)。

2026-10-12 T15 PR记录：用户要求“先保持待验收 创建pr”，已推送codex/t15-enemy-drops并创建[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43)，当前Open。T14 PR #42尚未合并，T15暂以codex/t14-shooting-ai为目标，只显示T15的36件改动，待#42合并后再改为main。#17继续Open / In Progress，当前图形PIE待验收、验收框保留未勾选；不关闭任务。原工程Editor构建、88项全量与最终11项复测/2项Game导航沿用已归档实测，本次仅PR与文档记录，未重复运行UE。本地继续develop，玩家原有资产改动未纳入。现有workflow仅自动检查main/develop目标，叠加PR按最终head手动运行Repository checks，实际结果另以GitHub检查页为准。以下段落保留历史。

2026-10-12 T15推进：三个BP_EnemyBase子类以青色三角棱柱D01、橙球D02、紫长条D03预告固定掉落，共用T14射击AI；生命死亡后只尝试一次，近处有效出生、真实物理Dormant、24额度回收/保护与回调世代取消统一接入。保存12件新资产与L_T15Drops静态导航，参数v0.16/Config和DEC34同步。原工程Editor构建通过，全量88项Editor Success（新增11/既有77）；仅测试夹具修正后新增11项再次Success、2项Game保存导航Success，导出errors/warnings=0。T15 #17保持Open / In Progress、当前Murdock图形PIE待验收；T14 #16仍待用户PIE，PR #42未追加本轮内容。本轮develop本地提交，无T15推送/PR。以下段落保留历史。 接入和四项PIE清单见[Enemy_Drops_Setup](Enemy_Drops_Setup.md)。T16–T27仍Todo。

2026-10-12 T14 PR记录：按用户要求已推送远程codex/t14-shooting-ai → main并创建[T14 PR #42](https://github.com/YYchainsAw/MothEffect/pull/42)，当前Open；T13 PR #41已合并。用户明确确认“尚未完成，保留待验收”，当前Murdock图形PIE不记通过，#16保持Open / In Progress；合并后仍未验收则转Pending Acceptance。原工程Editor构建、77项Editor与2项Game导航沿用2026-10-11源码678dc66的实际证据；首个PR head为1a18890，Repository checks已Success（run38189638920），本次仅同步交付文档与阅读版，最终head检查另按GitHub实际结果核对。本地继续develop，玩家原有蓝图改动保留。以下段落保留历史。

2026-10-11 T14推进：EnemyBase一套射击AI已接入视线/完整预警/共用P01、最后可见位置追踪、静态导航与停滞持续换点、共享生命/推力及幂等死亡。参数v0.15与Config同步，保存BP_EnemyBase和带掩体/平台/已构建NavMesh的L_T14AI。原工程Editor构建通过，全量77项Editor Automation Success（新增11/既有66），另一个实际加载保存场地的Game进程2项导航Success；当前Murdock图形PIE待验收，#16为Open / In Progress。接入见[Enemy_Setup](Enemy_Setup.md)，证据见[T14报告](../06_Test_Doc/T14_Report_2026-10-11.md)。T01–T13已Done，T15–T27仍Todo；本轮仅develop本地提交，不推送T14到T13 PR #41。以下段落保留历史。

2026-10-11 T13 验收完成：用户在五项清单后确认“测试通过 创建pr”，#15为Closed / Done，统一 WorldSubsystem 负责总道具/P01 登记、同类 Active 安全替换、累计闲置/警告与界外/重置清理。参数 v0.14 沿用上限与寿命，新增 Dormant Spent 反馈 0.25s；用户关闭编辑器后，修正Unity测试辅助函数重名；原工程Editor正式编译/链接成功，全量66项Automation Success（新增13/既有53），当前Murdock PIE按用户确认通过。接入和范围见 [Lifecycle_Setup](Lifecycle_Setup.md)、[T13 报告](../06_Test_Doc/T13_Report_2026-10-11.md)。T01–T13已Done，下一项T14；正式敌人容量、整局重开/跨波、最终场地安全盒、独立包与完整G2继续随后续任务验收。本次授权已独立推送远程codex/t13-lifecycle → main并创建[T13 PR #41](https://github.com/YYchainsAw/MothEffect/pull/41)，当前Open，本地继续develop。以下段落保留历史。

2026-10-11 T12 完成：用户确认“全部通过，可以记录验收”，五站自然连锁/布局对照及暂停恢复、结束清理的当前 Murdock PIE 已按用户确认归档，#14 为 Closed / Done。`bc3ce46` 交付源码、十项连锁测试和五站十四件普通装置的 L_T12Chains，Editor 构建与全量 53 项 Automation Success；证据和范围见 [T12 报告](../06_Test_Doc/T12_Report_2026-10-11.md)、[场景接入](Natural_Chain_Setup.md)。参数 v0.13 不变，T01–T12 已 Closed / Done，T13–T27 仍 Todo，下一项 T13；正式敌人/掉落、容量、整局重开、独立包与完整 G2 继续随对应任务验证。本次按用户授权独立提交 [T12 PR #40](https://github.com/YYchainsAw/MothEffect/pull/40)：远程 codex/t12-natural-chains → main，本地不切换 develop；依赖的 T10/T11 PR #39 已合入 main。以下段落保留历史。

2026-10-11 T11 完成：用户在探地碰撞排查后确认“通过 提交并pr”，#13 当前实现验收已补齐并同步为 Closed / Done。D01 在 `a5903f6` 交付，两项灰盒已保存；Editor 构建与全量 43 项 Automation 通过。最新 15:19 PIE 日志有三次成功起柱及正常结束，包含空中启动；图形表现、单次弹射和支撑按用户确认归档，见 [接入清单](Launcher_Setup.md)、[T11 报告](../06_Test_Doc/T11_Report_2026-10-11.md)。地面应为 WorldStatic，使用 BlockAll 或等效自定义配置，不能以 BlockAllDynamic 替代；保留 MothColumnSurface 标签。参数 v0.13，T01–T11 为 Closed / Done，T12–T27 仍 Todo，下一项为 T12。本轮用户授权推送 develop 并向 main 创建包含尚未合入的 T10/T11 的 PR；独立包、正式敌人/掉落、完整自然连锁和 G2 仍待后续验证。以下段落保留历史。

2026-10-11 T10 完成：用户在收到当前 PIE 清单后确认“测试通过”，#12 验收记录已补齐并同步为 Closed / Done。D02 单次爆炸与统一推力、灰盒 BP_DeviceBomb 在 `5692d76` 交付；此前本机 Editor 构建和全量 32 项 Automation 通过，九项新增覆盖爆心、遮挡、去重、实际速度、暂停清理与保存资产重新加载。确认范围见 [Bomb_Setup](Bomb_Setup.md)，结果见 [T10 报告](../06_Test_Doc/T10_Report_2026-10-11.md)。正式敌人、D01 组合及完整 G2 随对应任务补测；T01–T10 当前 Closed / Done，T11–T27 仍 Todo，下一项为 T11。以下 T09/早期段落保留历史。

2026-10-11 T09 完成：用户在收到当前角色 PIE 清单后确认“测试通过”，确认原文与范围已归档；GitHub #11 验收及验证记录已补齐，状态为 Closed / Done。T01–T09 当前均 Closed / Done，T10–T27 为 Open / Todo。助手此前完成 UE 5.8 Editor 增量构建与全量 23 项原生 Automation，全部成功，未预期 errors/warnings 为零；用户 PIE 确认与助手原生执行证据分别见 [T09 报告](../06_Test_Doc/T09_Report_2026-10-11.md)。正式胜负/重开随 T16 接入后补测，G0–G4 仍按完整证据验收。源码与文档本地提交，用户自行推送及创建 PR；原有 10/7–10/8 段落保留为历史。

2026-10-08 17:06 T08 复测：用户最新 Test Run 3 的 Devices 九项与 Projectiles 三项全为 Success，日志未再出现 World 缺少 EndPlay 警告，`23eb829` 收尾修正已复测。证据见 [T08 验证日志的修正后复测段](../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。下一步为 PIE TC03/TC04/TC05 与适用边界；G1 仍未验收，T07 边界仍按原范围保留。

2026-10-08 T08 局部验证：用户 Test Run 3 的 Devices 九项全部 Success（Emitter 三项 + 既有六项）；三个 Emitter 案例带 World 缺少 EndPlay 的清理警告。已在 `23eb829` 改为调用 UWorld::EndPlay，修正后编译/复测待用户执行；P01 三项与 PIE TC03–TC05 仍无结果，G1 未验收。原始记录见 [T08 验证日志](../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)，报告已按真实范围更新。本轮仅提交测试修正和文档，用户资产改动保留。

2026-10-08 T08 开发更新：用户要求继续 T08 并授权每完成一个小功能由助手本地提交。P01 已提交 `35643dc`；D03 世界方向、定时发射、出生净空、结束清理与六项新增 Automation 源码已接入，步骤见 [Emitter_Setup](Emitter_Setup.md)，[模板报告](../06_Test_Doc/T08_Report_2026-10-08.md) 保留未编译/未执行结果。继续使用当前 develop，未创建分支、PR 或推送，未修改 GitHub 状态。T08 编译、资产配置、Automation、TC03/TC04/TC05 与 G1 待用户执行；T07 未测边界不因继续 T08 而标记通过。

2026-10-08 T07 开发更新：用户增量编译成功，Automation Test Run 3 六项全部通过；用户明确确认仅 PIE 基础拾取、放下和投掷通过，输入/安全释放/表现与生命边界仍待完整实测。范围见 [Device_Interaction_Setup](Device_Interaction_Setup.md#8-2026-10-08-已验证范围)，原始摘录见 [T07 验证日志](../06_Test_Doc/Evidence/T07_Verification_2026-10-08.log)，[模板报告](../06_Test_Doc/T07_Report_2026-10-08.md) 已整理。助手未执行 UE，未创建分支或 PR，未修改 GitHub 状态；T08–T27 尚未在本轮推进。

当前记录（2026-10-08）：已建立六个 Milestone 和 27 条 T01–T27 Issue，首版提交 DDL 为 **2026-10-17**。此前核对 T01–T03 为 Closed / Done；T04 为 Closed / Pending Acceptance；T05 为 Closed / Done，[PR #34](https://github.com/YYchainsAw/MothEffect/pull/34) 已合并。T06 当前三项 Automation 与 PIE 状态/命中壳验收通过，待用户提交记录并走 PR 流程；本次未修改 GitHub 状态。T07 当前开发中，T08–T27 按此前 Todo 排期推进。每日 4–6 小时与少量原创机关、场景资产和 UI 已由用户确认；T01–T04 的环境、构建与证据尚未完整填写，G0–G4 仍待完整验收记录。操作见 [GitHub 开发流程](GitHub_Workflow.md)。

v0.9 玩家方案已经用户确认：首版不用 GAS，复用本地 Rifle 动画，C++ 行动状态与 AnimBP 移动状态分开，开火/换弹用上半身 Montage。T05 当前动画和边界 PIE 已由用户确认通过，结果见 [Weapon_Setup](Weapon_Setup.md)。T06 Test Run 4 三项均通过、PIE 当前分支通过，结果及此前失败保留在 [Device_Setup](Device_Setup.md)。完整案例随后续实现补测；最低玩家动画安排进 T04/T05/T07，T18 负责 UI，T20 负责少量原创资产与资源整合。[10/7 进度核查](Progress_Audit_2026-10-07.md) 保留实施本轮机关代码前的历史状态。

## 1. 如何使用这份计划

每天从最早未通过的门槛开始，只做依赖已完成的任务。任务先进入“进行中”，满足可观察完成标准并留下证据后才改为“完成”。一段录屏能证明几个规则时可以共用证据，不必重复制作材料。

玩法按 [游戏策划案](../02_Design_Doc/GDD/Game_Design.md)，规则按 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)，实现按 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)，关卡、AI、波次和资源按 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，所有数值以 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 为唯一基线。测试编号见 [测试计划与验收](../06_Test_Doc/Test_Plan.md)，G4 的平台步骤与审核状态见 [发布检查清单](../07_Release/Release_Checklist.md)。

单人开发允许顺序调整；门槛和任务依赖决定是否能够继续扩展。UI 布局和图标草稿可提前做；原创机关外壳和场景模块在 G2 后制作，按已预留的时段替换灰盒，不抢最后的测试与提交时间。

## 2. 时间预算与必须保住的内容

**2026-10-07 用户确认：每天 4–6 小时；首版加入少量原创机关、场景资产和 UI。** 10/7–17 共 11 个日历日，最多 44–66 小时；扣除 15% 缓冲后，净实施额度约 **37.4–56.1 小时**。这是按每天完整可用计算的上限，10/7 已经用掉的时间应扣除。开发机/目标机、干净编译、独立包和恢复证据另按原任务补档。

原 T06–T27 粗估 44–71 小时；T18 从 2–3 调为 4–6 小时，T20 从 2–3 调为 6–8 小时后，T06–T27 原始预算为 **50–79 小时**。10/7 初次排期另暂留 T05 剩余与 T01–T04 补档/核验 **2–5 小时**，当时合计 **52–84 小时**。本轮 T05 已完成、T06 代码已推进，后续按实际剩余验收与补档重新估时；已有代码和前置未实测前不直接按整个任务扣除工时。

这意味着：**多数天只有 4 小时无法覆盖完整范围；接近每天 6 小时、返工较少并采用最小资源规格，才可能完成工作下界。** 下面的每日表安排 55 小时有效工作、其余最多 11 小时留作分散缓冲和 10/17 阻断修复；它是紧凑目标，不是完成承诺。每天按剩余分支重估，不把已过去的日期、已关闭任务或 CI 通过折算为可用时间。

| 预留内容 | 有效小时 | 纳入任务 | 边界 |
|---|---|---|---|
| UI 布局、三机关图标、HUD、教程与共用菜单面板 | 4–6 | T18 | 制作与接入都计入；先占位数据，后接真实血量/弹药/持物/波次/结果 |
| 少量原创机关和场景资产、整合与来源记录 | 6–8 | T20 | 三种简洁机关外壳、一个可复用掩体模块、一个符文/灯光装饰；复用人物与动画 |
| 必要战斗反馈 | 2–3 | T19 | 枪口/命中、启动/方向、危险/过期提示；不做多套复杂特效 |
| 独立包内部试玩、盲玩、调参、稳定性和候选回归 | 9.5–15.5 | T21–T25 | 与各任务局部验证分别安排，不因美术延后而取消 |
| 投稿材料与首次上传/自测/提审 | 2.5–4.5 | T26–T27 | 优先 10/16 首次提交，10/17 留给修复与补提交 |

这些预留已经计入任务表，**不再次加总**。T01–T27 修订总粗估为 59.5–93.5 小时，包含已经过去的工作，只用作原计划对照。T28 是提交后按审核反馈触发的 4–8 小时，不计入 10/17 前预算；选做开发日志另需 2.5–5 小时，有意参与时必须从现有预算扣除。

必须保住：一把主武器、三种装置、拾取投掷与空中射击启动、真实规则连锁、一张主竞技场、一套射击 AI 的三种掉落外观、三波胜负与重开、可读 UI/必要反馈和可启动 Windows 包。原创资产先做到可辨认且可整合，精细打磨与额外装饰可压缩。第二套 AI、额外波次、候选机关、完整轨迹预览、多地图、Boss、成长树、移动端、联机继续延期。

## 3. 五个开发门槛

| 门槛 | 目标日期 | 必须观察到的结果 | 证据与失败处理 | 当前验收记录 |
|---|---|---|---|---|
| G0：环境与独立打包 | 10/8 补齐 | 本机能打开实际安装的 UE 5.8 项目，完成 C++ 编译；模板基础移动可用；Windows 包在关闭编辑器后能进入测试房间 | 记录实际引擎补丁号、工具链、构建标识和启动截图。未通过时先解决环境与打包，不开始批量内容制作；若安装版本不可用，记录问题再决定版本，不默默更改用户版本 | 未验收 |
| G1：投掷与空中启动 | 10/9 | D03 能拾取、投掷、空中射击启动，沿命中射线方向发射并继续受重力；练习约 2 分钟后，安全测试房 10 次投掷中至少 8 次能完成空中启动，方向无误 | 保存 TC03–TC06、TC12、TC17 的录屏和计数。命中率是内部原型指标；失败先改可读性、相机、投掷和输入，不扩大敌人数量 | 未验收 |
| G2：真实规则连锁 | 10/11 | D01、D02、D03 均完成；至少两种不同机关连锁由弹丸、推力和位置自然发生；换摆放位置会改变结果，不依靠关卡专用连锁脚本 | 录下“发射弹启动另一装置”和“弹射/爆炸改变道具位置后被弹道启动”的过程；TC07–TC16 通过。失败暂停原创模型与场景装饰，修正命中上下文或推力规则；已预留的必要 UI 接入保留 | 未验收 |
| G3：可试玩完整闭环 | 10/13 内部；10/15 完整 | 教程提示 → 三波战斗 → 胜利，或者死亡 → 失败 → 重开；三种敌人外观能预告对应掉落；无残留对象与定时器 | M4 用独立包完成 TC20–TC25 内部流程及玩家专项；M5 补齐 TC26 外部盲玩后再验收完整 G3，保存对应记录。失败保留一个 AI，调整出怪与场地，先实现完整闭环 | 未验收 |
| G4：提交首版并跟踪审核 | 10/17 | 本地候选包完成阻断项验收；厂商认证与投稿资料已核对；由开发者在实际后台上传、完成自测并提审，保存页面状态和包体标识 | 不能把“已上传”记成“已过审”。若尚有阻断缺陷不提审；10/18–20 只处理审核与阻断修复，最终应在官方截止前通过审核 | 未验收 |

G0 中的模板、工具链和独立包证据需对应用户实际安装环境与构建；T02 已关闭，但完整记录尚未补档。G4 的上传、平台自测、提审及过审仍需真实后台状态，本次 GitHub/Docs 配置没有执行这些操作。

## 4. 日期安排

以下保留原六个 Milestone 与任务归属，依据 10/7 核查重排目标日期。原 M1/M2/M3/M4/M5 目标为 10/5、10/7、10/9、10/12、10/15，本轮调整为 10/8、10/9、10/11、10/13、10/16；M6 保持 10/17。10/15 完成外部试玩与调参，10/16 先通过候选回归再首次提交。原目标未通过的事实保留，改日期不代表补作验收。

| Milestone | 截止（2026） | 任务 | 阶段完成条件 |
|---|---|---|---|
| [M1 · 可运行的射击基础](https://github.com/YYchainsAw/MothEffect/milestone/2) | 2026-10-08 | T01–T05 | 补齐 G0：核验环境、干净编译、独立 Windows 包启动和恢复流程；移动、瞄准、步枪、手动换弹、生命与一次死亡可用，相关适用测试有证据。提前核对平台提交条件与认证状态。 |
| [M2 · 投掷与空中激活](https://github.com/YYchainsAw/MothEffect/milestone/3) | 2026-10-09 | T06–T09 | 通过 G1：机关状态与命中上下文、拾取、安全放下、投掷、D03/P01 可用；命中方向锁定且机身受重力；熟悉后十次至少八次空中启动成功；同次按压不会同时投掷与射击。 |
| [M3 · 三机关与自然连锁](https://github.com/YYchainsAw/MothEffect/milestone/4) | 2026-10-11 | T10–T13 | 通过 G2：D01/D02/D03 成立，至少两种连锁由真实弹丸、推力与位置产生，改变布局会改变结果；重复命中、容量和清理有测试结果。通过后冻结规则与接口。 |
| [M4 · 三波可玩闭环](https://github.com/YYchainsAw/MothEffect/milestone/5) | 2026-10-13 | T14–T18、T21 | 完成 G3 内部流程测试：一套射击 AI、三种固定掉落外观、一张主场地、教程与 HUD/共用菜单；独立包可完成三波胜利、死亡失败与重开，对象和计时器清理有证据。TC26 外部盲玩在 M5 补齐后再验收完整 G3。 |
| [M5 · 试玩与候选包验收](https://github.com/YYchainsAw/MothEffect/milestone/6) | 2026-10-16 | T19–T20、T22–T25 | 完成必要反馈、少量原创资产与资源整合；至少一位不了解设计的人盲玩，补齐 TC26 后验收完整 G3；按实际问题调参并回归；完成压力、稳定性和候选压缩包验收，冻结可交付候选包及证据，通过后才首次提交。 |
| [M6 · 最终包与提交](https://github.com/YYchainsAw/MothEffect/milestone/7) | 2026-10-17 | T26–T27 | 最终包通过阻断项验收，物料与资源说明齐备；完成上传、平台自测与提审，保存版本、提交时间和实际状态。10/16 优先完成首次提交，10/17 留给阻断修复、回归和补提交。 |

### 4.1 每日工作时段

工时含当日相关局部验证；实际顺序由依赖与结果决定。下表是接近每天 6 小时时的 55 小时实施目标；额外可用时间优先用于当日缺陷，10/17 不安排新增内容。

| 日期 | 有效小时 | 核心/验收重点 | UI、美术与反馈时段 |
|---|---|---|---|
| 10/7 | 6 | T05 Montage 与武器分支；开始 T06 和 T07；核对现有构建/恢复记录 | 先确定 UI 和机关外壳清单，本日不增加模型任务 |
| 10/8 | 6 | 完成拾取/安全释放，推进 D03/P01；补齐 G0 独立包与恢复证据 | 使用几何占位验证，不打磨外观 |
| 10/9 | 5 | T08/T09 联调，通过 G1；若未通过立即占用缓冲修核心 | T18：1 小时，HUD/菜单布局和三机关图标草稿 |
| 10/10 | 6 | D02、D01 基本效果；可用场地灰盒与导航准备 | T18：1 小时，统一字体、颜色、图标与提示样式；使用占位数据 |
| 10/11 | 5 | 两种自然连锁、容量与清理，G2 验证 | T20：1 小时，仅在 G2 通过后确定外壳轮廓和模块；未通过则修核心 |
| 10/12 | 6 | 接入射击 AI、三种掉落；推进三波/场地 | T20：2 小时，三种机关简洁原创外壳；共享材质与背包标记 |
| 10/13 | 6 | 三波/胜负/重开与 T21 独立包内部验收 | T18：2 小时，接入真实 HUD/教程/暂停/结果数据；T20：1 小时，可复用掩体 |
| 10/14 | 5 | 收尾资源整合，解决内部试玩阻断项 | T20：2 小时，符文/灯光装饰、替换与记录；T19：2 小时，必要反馈 |
| 10/15 | 5 | 一人盲玩、针对问题调参/回归、性能与稳定性，验收完整 G3 | 冻结 UI 和资产数量，只修提示/辨识问题 |
| 10/16 | 5 | 候选压缩包回归、物料、上传、自测与提审；补齐 M5/M6 记录 | 从同一实机版本取截图/视频，不追加展示资产 |
| 10/17 | 缓冲 | 阻断修复、受影响回归和必要补提交，保存最终版本及后台状态 | 只修已有 UI/资产影响操作或打包的问题 |

T18 的 4 小时与 T20 的 6 小时是各自下界，实际需要 6/8 小时时从分散缓冲拿时间；不从测试或 10/17 截止之后借时间。UI 布局可早做，真实数据、暂停和结果必须等对应逻辑存在后接入；T20 制作与最终替换分别核验，模型外观不更改已验证的物理根、碰撞和机关方向。

10/16 优先完成首次提交，10/17 留给阻断修复、回归与补提交。T28 为首版提交后由实际审核反馈触发的条件任务，尚未创建 Issue，不借该窗口补首版核心玩法。

官方截止与审核要求继续按 [发布检查清单](../07_Release/Release_Checklist.md) 及实际后台核对；内部 10/17 DDL 与平台审核完成状态分别留证。

## 5. 任务清单

估时单位是有效工作小时，均含该任务局部验证，不含大范围返工。任务依赖表示执行前需要成立的条件；不必为了表格逐项增加形式化会议。类别“核心”不能删除，“可压缩”可减轻实现或验证规模，“选做”仅有时间才执行。

| ID | GitHub Issue | 类别 | 任务 | 前置依赖 | 粗估小时 | 可观察完成标准 | 状态（10/7） |
|---|---|---|---|---|---|---|---|
| T01 | [#4](https://github.com/YYchainsAw/MothEffect/issues/4) | 核心 | 核对文档、时间与决策 | 无 | 0.5–1 | 在 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 中记录每日可用时间、PC 基线、版本环境待验项；确认规则与参数唯一来源，发现矛盾先修文档 | Done；4–6 小时已确认，环境/平台明细待补 |
| T02 | [#3](https://github.com/YYchainsAw/MothEffect/issues/3) | 核心 | 核对现有 UE 5.8 模板工程、干净编译与首次 Windows 打包 | T01 | 2–4 | 复用当前工程 [MothEffect.uproject](../../MothEffect.uproject)；编译成功、模板基础角色可移动、关闭编辑器后包体能启动，G0 有记录 | Done；找到 UE 5.8.0/PIE 日志，干净编译/独立包待证据 |
| T03 | [#5](https://github.com/YYchainsAw/MothEffect/issues/5) | 核心 | 目录、版本管理与恢复 | T02 | 1–1.5 | 项目按 [代码与资产命名规范](../03_Code_Standard/Coding_Conventions.md) 目录归档，并遵循 [版本管理规范](../03_Code_Standard/Version_Control.md)；源代码与资产有可恢复版本记录；从最近备份取回一个测试文件，且不把缓存当源文件 | Done；忽略/LFS 已核对，模板恢复与实际备份恢复待证据 |
| T04 | [#6](https://github.com/YYchainsAw/MothEffect/issues/6) | 核心 | 测试房、越肩相机、移动/转向与基础动画 | T02 | 3–4 | 镜头 Yaw 转向与八方向移动、跳跃/跑步/瞄准；最低 AnimBP/BlendSpace 可用，瞄准与冲刺互斥；墙边相机可用；TC34/TC35 基础分支有证据 | Pending Acceptance；已关闭，基础项已勾选，证据待补 |
| T05 | [#7](https://github.com/YYchainsAw/MothEffect/issues/7) | 核心 | W01、玩家行动状态、换弹与生命伤害/上身动画 | T04 | 3–4 | 静止/移动射击、双段枪口检测、手动换弹、血量与一次死亡成立；ARifle 统一射速/弹匣/任务；上身 Montage 不锁腿部；TC24/TC36/TC38 武器分支有证据 | Done / Closed；用户确认动画与边界 PIE 通过，PR #34 已合并 |
| T06 | [#8](https://github.com/YYchainsAw/MothEffect/issues/8) | 核心 | 装置基类、状态与命中上下文 | T02 | 1.5–2.5 | Dormant、Held、Active、Spent、Destroyed 转换可记录；激活幂等；重复命中不会重复创建效果 | 当前范围验收通过；Automation Run 4 三项 Success，PIE 激活/重复命中/Spent 通过；待用户提交记录并走 PR，GitHub 状态本次未修改；Held 操作随 T07 接入 |
| T07 | [#9](https://github.com/YYchainsAw/MothEffect/issues/9) | 核心 | 拾取、放下、投掷与最低持物表现 | T04、T06 | 3–4 | E 拾取/放下、Held 左键只投掷；安全检测失败保留 Held；成功释放到 Dormant，玩家进入投掷恢复；持物挂点/短释放可读；TC06/TC17/TC37 有证据 | 部分验证通过；增量构建成功，六项 Automation 与 PIE 基础拾取/放下/投掷通过；其余适用边界与表现待验收，未改 GitHub 状态 |
| T08 | [#10](https://github.com/YYchainsAw/MothEffect/issues/10) | 核心 | D03 与基础 P01：空中启动方向锁定 | T05–T07 | 3–5 | 投掷中射击启动，沿命中方向持续发射；装置继续落下；达到 G1 可用性指标 | 最新 Run 3 十二项 Automation 全通过，World 收尾无警告；PIE TC03–TC05 与 G1 待验收，GitHub 状态未改 |
| T09 | [#11](https://github.com/YYchainsAw/MothEffect/issues/11) | 核心 | 输入切换、恢复、中断与单次激活边界 | T08 | 2–3 | 投掷松键与恢复后的新按压才射击，提前按下不缓存；失焦/暂停/动画中断/取消不留锁或任务；Held 免启动但玩家受伤；Active 不拾取/重启；TC06/TC12/TC17/TC31/TC37/TC38 适用分支通过 | Done / Closed；九项原生边界测试与全量 23 项回归通过，当前角色 PIE 用户确认通过；[报告](../06_Test_Doc/T09_Report_2026-10-11.md) 已归档，正式胜负/重开联调随 T16 补测 |
| T10 | [#12](https://github.com/YYchainsAw/MothEffect/issues/12) | 核心 | D02 爆炸、伤害与推力 | T05、T06 | 2–3 | 地面/空中在激活位置爆炸；敌我受到伤害；可移动道具受推力；爆炸不直接启动其他机关 | Done / Closed；源码、灰盒与参数已交付，九项新增及全量 32 项 Automation 通过；用户确认[当前 PIE](Bomb_Setup.md)通过，正式敌人/D01 联调随后续任务补测 |
| T11 | [#13](https://github.com/YYchainsAw/MothEffect/issues/13) | 核心 | D01 探地、升柱与弹射 | T06、T07 | 3–5 | 向世界竖直下方探地，检查最终柱体静态净空；伸出阶段逐对象单次弹射；停留期间净空后开实体支撑/阻挡，收回前关闭，不夹住角色或道具；TC09、TC10、TC33 通过 | Done / Closed；源码、两项灰盒和全量 43 项 Automation 通过，用户确认[当前 PIE](Launcher_Setup.md)通过，另有三次成功起柱日志；正式敌人/完整连锁随对应任务补测 |
| T12 | [#14](https://github.com/YYchainsAw/MothEffect/issues/14) | 核心 | P01 统一触发与自然连锁 | T08、T10、T11 | 2–4 | 敌人与机关弹丸按统一规则命中、启动、伤敌我；生成者忽略自己的弹丸；两种非脚本连锁满足 G2 | Done / Closed；全量 53 项 Automation、真实 P01 与两类运动连锁及阴性对照通过；用户确认五站与暂停/清理[当前 PIE](Natural_Chain_Setup.md)通过，正式敌人及完整 G2 随对应任务补测 |
| T13 | [#15](https://github.com/YYchainsAw/MothEffect/issues/15) | 核心 | 容量、过期、Spent 与清理 | T06、T12 | 1.5–2.5 | 按 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 与 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 管理总道具/活跃数上限、警告和清理；总道具容量回收不删除 Held/Active，同类 Active 上限按规则结束最早装置；多次重开计数回到基线 | Done / Closed；原工程Editor正式构建/链接与全量66项Automation通过，用户确认[当前PIE](Lifecycle_Setup.md)五项清单通过；完整整局/正式敌人分支随后续任务验收 |
| T14 | [#16](https://github.com/YYchainsAw/MothEffect/issues/16) | 核心 | 一套射击 AI：移动、视线、预警、P01 | T05、T12 | 3–5 | 敌人能看见/追踪玩家，隔墙不无视遮挡射击；开火前提示；掉落前可被击杀；攻击间隔走配置 | Merged / Closed（#42合并自动关闭#16）；原工程77项Editor/2项Game导航通过，当前角色PIE此前未完成，待补验收记录 |
| T15 | [#17](https://github.com/YYchainsAw/MothEffect/issues/17) | 核心 | 三种敌人外观与确定掉落 | T14、T10、T11 | 1–2 | 三种简单外观与符号分别对应 D01/D02/D03；死亡位置掉落一次，掉落对象状态正确，不做随机表 | In Progress / Open；#43/#44已合并；固定掉落保存实例修复与W01击杀/匹配掉落/拾取已由用户确认，容量/更多地形等边界继续待验收 |
| T16 | [#18](https://github.com/YYchainsAw/MothEffect/issues/18) | 核心 | 三波导演、胜负与重开 | T13、T15 | 2–3 | 出怪和波次内容符合 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)；玩家死亡结束战斗；最后一波结束胜利；重开清对象、事件、计时器与波次状态 | In Progress / Open；[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45) → main；原生/Game通过，用户要求保持当前PIE待校验 |
| T17 | [#19](https://github.com/YYchainsAw/MothEffect/issues/19) | 核心 | 一张主竞技场灰盒、掩体与导航 | T14 | 2–3 | 按 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 的三区和尺度落地；可完成战斗/投掷/弹射；导航及出生点无卡死；保留大型场地意图，不追加第二张地图 | In Progress / Open；[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45) → main；15条保存路线/8项主场地Game通过，用户要求四组当前PIE待校验；见[Arena_Setup](Arena_Setup.md) |
| T18 | [#20](https://github.com/YYchainsAw/MothEffect/issues/20) | 核心 | UI 布局/图标、HUD/教程与共用菜单接入 | T07、T16、T17 | 4–6 | 完成生命/弹药/持物/波次与三机关图标，按真实状态显示教程、开始/暂停/胜负/重开；初始值正确，暂停输入模式与鼠标切换可用，目标分辨率不裁切；最低操作观察通过 | Open / In Progress；[PR #46](https://github.com/YYchainsAw/MothEffect/pull/46)已合并；编辑接口/基础骨架本地完成，具体UI用户制作；五组硬件 PIE 待验收，见 [UI_Setup](UI_Setup.md) |
| T19 | [#21](https://github.com/YYchainsAw/MothEffect/issues/21) | 可压缩 | 提升命中、启动、方向与警告反馈 | T12、T18 | 2–3 | 枪口/命中、三装置类型、方向箭头、Active/Spent、过期警告可区分；最低反馈保留，多层粒子与音色变体可删除 | 默认反馈实现与配置已接入；原生/主地图结果见[T19报告](../06_Test_Doc/T19_Report_2026-10-13.md)，四组用户PIE待验收；#21 Open / In Progress |
| T20 | [#22](https://github.com/YYchainsAw/MothEffect/issues/22) | 可压缩 | 少量原创机关/场景资产、整合与来源记录 | T17 灰盒；最终整合依赖 T19 | 6–8 | 完成三种简洁原创机关外壳、一个可复用掩体和一个符文/灯光装饰；统一色形/材质，敌人标记与掉落对应；保留来源/制作记录，核对模板依赖恢复；不改变玩法碰撞与方向，替换后验证操作与打包 | 原型/接口已完成并本地提交；17资产与原生/主地图回归通过，见[T20报告](../06_Test_Doc/T20_Report_2026-10-13.md)；五组用户验收/缺依赖恢复/独立包待执行，#22 Open / In Progress |
| T21 | [#23](https://github.com/YYchainsAw/MothEffect/issues/23) | 核心 | 内部完整打包试玩与 G3 内部验收 | T09、T13、T16–T18 | 2–3 | 从独立包通关三波并死亡重开，TC20–TC25 和玩家专项适用分支有结果与录屏；TC26 由 T22 完成，通过前完整 G3 保持待验收 | Todo |
| T22 | [#24](https://github.com/YYchainsAw/MothEffect/issues/24) | 可压缩 | 外部盲玩反馈 | T19、T21 | 1.5–2.5 | 至少一名不了解设计的人盲玩并执行 TC26，保存误操作和理解观察；失败结果交给 T23，复测通过后结合内部流程记录完整 G3；本任务不等待修复才交接 | Todo |
| T23 | [#25](https://github.com/YYchainsAw/MothEffect/issues/25) | 核心 | 根据证据调整参数与教学 | T22 | 2–3 | 只改已暴露问题；更新 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 与资源参数，并回归相关测试；玩家可理解发射方向与危险来源，记录改前/改后 | Todo |
| T24 | [#26](https://github.com/YYchainsAw/MothEffect/issues/26) | 核心 | 性能压力、稳定与边界修复 | T13、T21、T23 | 2–4 | 在指定基准机器与最大配置密度下录性能；重开、退出、失败流程没有泄漏和崩溃；记录达标或具体问题，不能只写“流畅” | Todo |
| T25 | [#27](https://github.com/YYchainsAw/MothEffect/issues/27) | 核心 | 候选包与提交阻断回归 | T20、T24 | 2–3 | 候选包从最终压缩包重新解压；[测试计划与验收](../06_Test_Doc/Test_Plan.md) 中阻断提交的测试通过；版本标识与证据一致，无仅在编辑器成立的结果 | Todo |
| T26 | [#28](https://github.com/YYchainsAw/MothEffect/issues/28) | 可压缩 | 参赛物料、玩法简介与资源说明 | T19、T25 | 1.5–2.5 | 按 [发布检查清单](../07_Release/Release_Checklist.md) 与后台实际要求准备图标、宣传图、实机截图/视频、说明与资源记录；展示投掷空中启动及自然连锁；剪辑装饰可省 | Todo |
| T27 | [#29](https://github.com/YYchainsAw/MothEffect/issues/29) | 核心 | 开发者手动上传、自测、提审与状态记录 | T25、T26、厂商认证通过 | 1–2 | 按 [发布检查清单](../07_Release/Release_Checklist.md) 的 PC 检查顺序核对最终包、平台自测和投稿信息；保存提交时间/包体版本/状态；不把等待审核算成有效工作小时 | Todo |
| T28 | 未创建；按反馈触发 | 核心 | 审核意见与阻断缺陷修复窗口 | T27 | 4–8，另留 | 留存审核意见；只修改必须问题；新包执行受影响回归与 [发布检查清单](../07_Release/Release_Checklist.md)；实际状态为“审核通过”后才能结束 | 未创建；提交后按审核反馈触发 |

表中多个依赖写成区间表示包含区间内全部任务，例如 T16–T18 包含 T16、T17、T18。T14、T17、T18 应围绕最小可用场地交错实施。T18 的布局/图标可提前做，依赖成立后接真实数据；T20 的外壳制作在 G2 后开始，最终整合仍依赖场地与反馈成立。

## 5A. 候选扩展的评估入口

[候选机关道具](../02_Design_Doc/GDD/Device_Candidates.md) 已收录牵引核心、诱光灯、折光棱镜、静滞锚和换位匣，状态均为待选制作。本次不增加 T29 等实施任务，不计入 T01–T28 的工时、日期或门槛。

若决定制作其中某项，先核对 G3 完整体验与剩余提交缓冲，再记录选定范围、额外工时和受影响的规则及测试；冻结新功能及审核修复窗口继续按现有日期执行。没有可用工时则保留候选，不影响 D01–D03 的核心交付。

## 5B. 已确认玩家方案的实施顺序

先执行最早未验收的 G0；日期仍是建议目标，不因设计确认而认定 G1 已在目标日完成。玩家方案的权威接口/状态定义在 [技术设计第 3 节](../02_Design_Doc/TDD/Technical_Design.md#3-越肩射击与输入)，操作规则在 GDD；此处只安排执行。

1. T04：核验本地动画骨架/属性，配置越肩相机与镜头 Yaw 转向，接入八方向 BlendSpace 和基础腾空状态机；先验证侧移、跳跃与瞄准/冲刺。
2. T05：实现 Ready/Reloading/Dead 与 Rifle/Health，接入 Fire/Reload 上身表现；T06 保持机关自己的状态机，避免混入玩家行动。
3. T07–T09：接入 Carrying/ThrowRecovery、单次按压归属、安全释放、持物/释放反馈及取消收尾；换弹中拾取在武器与机关均存在后联调。验证 D03 空中启动达到 G1。
4. T10–T12：加入爆炸/弹射，补测 TC35 的直接腾空与落地可操作分支，达到真实连锁 G2。
5. T14–T18/T21：敌人掉落、三波、HUD、胜负与重开，补齐 TC34–TC38 全部适用分支并验收 G3；T20 按预留时段完成少量原创资产与整合，T23 只调整已暴露的问题。

不增加 GAS 接入任务或候选机关工时；专项测试随相关实现执行，不另叠一套完整测试工程。新增瞄准速度/空中控制仍是未实测起点，不能凭文档宣称手感成立。

## 6. 超时后的取舍顺序

每天结束时重算“剩余净时间”和“剩余任务粗估”，包含尚未解决的缺陷。剩余工作下界都超过剩余时间时，当天减范围，不能等到最后两天。

1. 先取消所有 P1、额外敌人行为、额外波次和额外道具；这些本来就未列入首版。
2. T20 先取消额外装饰、复杂材质和精细打磨，保留三种简洁原创外壳、一个掩体模块与符文标记的最小目标；T18 保可读 HUD/教程/结果，T19 保方向/伤害/激活反馈。若工作下界仍超预算，明确记录首版原创场景资产延期，不能暗中挤掉核心或提交测试。
3. T22 保留 1 人盲玩，不扩充人数；T26 用清楚的原始实机视频和截图，少做剪辑。不得删除外部操作观察或平台要求的物料。
4. 主竞技场保三个可辨认区域和可用投掷/弹射空间，用更少掩体降低导航、碰撞和装饰工作；仍是一张主竞技场，不增加关卡。
5. 降低同时敌人数、同时活跃机关及视觉粒子密度，但须同步修改 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)，并回归三波节奏和上限测试；保持三类道具、三波闭环与真实连锁。
6. 10/9 G1 或 10/11 G2 未通过时，暂停原创模型/装饰、重算后续节点，优先修核心；必要 UI 仍随逻辑接入。不把未达到核心标准的版本包装为完成；更新时间、未实现项和风险。

如果实际每天只有 2–3 小时，须从当前日期重新计算剩余时间，并优先验证 G0–G2；已过去的日期和已关闭任务不能直接折算为仍可使用的工时。根据真实剩余分支和预算重估三波参赛版，保留可运行核心原型与真实连锁，按上述顺序压缩可选表现。

## 7. 开发日志与过程证据

以下日志是选做，只有争取“全程参与奖”时才进入奖项计划。官方该奖项还要求按时有效投稿、完整核心循环和评审期至少 50 位有效试玩；五篇日志本身不保证获奖或达到试玩门槛。日志不是所有参赛者的强制条件。[官方答疑](https://www.taptap.cn/moment/851888213915075734)

| 建议日期 | 日志主题 | 应来自哪些真实证据 | 状态 |
|---|---|---|---|
| 10/9 | 为什么空中启动值得做；第一次操作验证 | G1 录屏、成功率、误操作与实际调整 | 未撰写；选做 |
| 10/11 | 三个装置如何产生连锁 | G2 两种连锁、统一规则与失败案例 | 未撰写；选做 |
| 10/13 | 敌人死亡位置如何改变下一波 | G3 三波闭环、场地与掉落布局 | 未撰写；选做 |
| 10/15 | 玩家第一次试玩暴露了什么 | 经同意使用的真实反馈、可理解性改动前后 | 未撰写；选做 |
| 10/17 | 首版边界与提交准备 | 实际候选包、已通过测试与仍保留的限制；尚未提审时明确写“准备中” | 未撰写；选做 |

单篇粗估 0.5–1 小时，避免额外拍摄一套素材。未完成门槛时，日志写真实问题与尝试，不能为了日期写成已成功。本文档只安排日志，不对外发布任何内容。

## 8. 每日结束时保存的最小记录

每天保存：当日构建/提交标识、完成任务、未通过测试、最重要的一条发现、次日第一个任务。规则发生变化时按 [文档维护与变更流程](Documentation_Workflow.md) 在 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 与 [变更日志](Changelog.md) 中记录原因、关联任务/测试，并让 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)、[技术设计](../02_Design_Doc/TDD/Technical_Design.md)、[关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)、[玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 保持一致。重复出现的未知问题进入缺陷记录，不另写一套与代码脱节的长文档。

当前进度快照见本文件开头和任务表；[10/7 进度核查](Progress_Audit_2026-10-07.md) 为历史。T13容量/闲置/清理的原生与用户当前PIE证据已归档，#15为Closed / Done；既有环境与完整门槛的缺失记录继续补档。完整 TC 与 G0–G4 不因 Issue 关闭或静态 CI 成功而自动通过；候选包、性能、外部试玩和平台状态仍需实际结果。
