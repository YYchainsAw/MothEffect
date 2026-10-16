# Moth Effect（飞蛾效应）：开发文档入口

版本 v0.61 · 2026-10-17 · 单人 UE 5.8 C++ 与蓝图混合开发

2026-10-17 首版游戏内流程：独立 `L_MainMenu` 城市机械庭院使用固定镜头和淡入，点击开始才进入 SoulCity，暂停／结算可返回独立入口或直接重开。菜单内无玩家、观战角色和敌人波次。设置、胜负统计与制作人员页面接入，独立教学入口改为开局基础提示；音量、鼠标灵敏度、画质与显示选择可保存，三波补充目标提示和机关击杀反馈。实现、操作与验证范围见 [Release_Flow_Setup](05_Development_Guide/Release_Flow_Setup.md)。

2026-10-16 Niagara 特效接入：八类实际开火、命中、机关与拾取事件统一连接项目 FX，暂停与结算清理沿用反馈系统。原工程构建、19 项 Editor 专项及八类实际 D3D12 模拟/暂停恢复/死亡清理通过，九件目标资产已写回；用户准星与 HUD 保持，整局、性能和独立包待验收。调节入口与证据见 [Niagara_Setup](05_Development_Guide/Niagara_Setup.md)。

2026-10-16 Slate 界面接入：用 Colorosse Slate / Kenney 重做 Designer HUD 和菜单，左下角玩家生命、敌人头顶生命、弹药/换弹、波次与交互提示已连接真实数据。旧原生布局与绘制移除，用户准星包 SHA 及 HUD 内 15 个事件图节点/连线保持。原工程 Editor 构建、三项 UI Editor 专项（2 成功、1 带警告成功）、实际伤害/菜单/暂停/重载专项和五张 D3D12 画面通过；手动整局、性能与独立包仍待验收。当前编辑入口见 [Slate_UI_Setup](05_Development_Guide/Slate_UI_Setup.md)，下方旧 UI 记录保留历史范围。

2026-10-16 敌人外观统一为 Drongo：保留三类敌人、固定掉落及当前射击 AI，项目动画蓝图复用原包移动/跳跃/上半身开火，移除演示玩家依赖并接入死亡动画。资产、验证与重载步骤见 [Drongo_Enemy_Setup](05_Development_Guide/Drongo_Enemy_Setup.md)。

2026-10-16 选定资源导入：Slate / Kenney 的 3,234 张贴图与 Wraith 已导入；用户后补的 Niagara Examples Pack、Drongo 也已核对，Minions 按用户要求暂缓。两包 2,582 件原文件经隔离检查保持 SHA 一致，Drongo 两件动画蓝图内存编译通过，15 个 Niagara 系统加载通过。导入阶段未改 HUD、玩家或敌人外观；后续 Drongo 接入见上段。具体位置、原包警告与验证边界见 [Asset_Import_Setup](05_Development_Guide/Asset_Import_Setup.md)，选型依据见 [资源清单](05_Development_Guide/Asset_Shortlist_2026-10-16.md)。

2026-10-16 新增 [本期开发者日志](05_Development_Guide/Developer_Log_2026-10-16.md)：整理10月14日至16日的道具重构、SoulCity战场和正式准星进展，记录击杀反馈、实际战斗、资源与性能风险，并列出下一期UI收尾、整局试玩及Windows候选包计划。日志引用已有验证，不新增玩法通过结论。

2026-10-15 SoulCity紧凑街区：`L_Arena_SoulCity` 在原多层战场内增加七栋封闭建筑，形成街巷、转角和高台遮挡；四面连续外围楼群配合城市远景卡片收住视野。地图打开与编辑见 [SoulCity_Arena_Setup](05_Development_Guide/SoulCity_Arena_Setup.md)，实际碰撞、导航、三波与八视角结果见 [报告](06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。原HUD和玩家资产保留，手动战斗、实际AI追击与性能待验收。

2026-10-15 Soul City 场内原生资产替换：`L_Arena_SoulCity` 保留84×72m、-2.4/0/3.6/7.2m四层与六条连接，场内主体改为原生通风设备、法兰管材、金属墙、完整楼板、台阶与护栏。17组掩体的位置保留，设备/管材/护栏碰撞按实际网格匹配；外观沿用原材质，独立检查主体可见方块替换。打开见 [SoulCity_Arena_Setup](05_Development_Guide/SoulCity_Arena_Setup.md)，资产/20导航/21支持/脚本三波及五视角证据见 [报告](06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。用户HUD和原地图保留，手动战斗与性能待验收。

2026-10-14 贴图准星：用户已导入 `T_CH001`、`T_HM_002`、`T_KI_001`，C++ 已补入敌人实际命中/击杀快照和 HUD 蓝图通知，旧 HUD 与菜单继续默认创建。蓝图图层、动画及接线由用户制作，步骤见 [自定义准星接入](05_Development_Guide/Crosshair_Setup.md)。按用户选择先交付源码和说明，完整编译、九项新增专项执行及 PIE 仍待进行，未修改用户资产；此前道具重构的验证范围保持原记录。

2026-10-14 当前进入道具基础重构，入口为 [Device_Refactor_Setup](05_Development_Guide/Device_Refactor_Setup.md)：地面固定且保留查询、显式推力/弹射再起飞、D03 锁入射 XY 水平方向、共用激活延迟及 D02 爆炸触发无遮挡 Dormant。规则 v0.12、参数 v0.19 与 DEC41–DEC44 已同步；当前 `refactor` 分支，代码分段本地提交 `f6d6b2a` / `f91db81`。新建 [#48](https://github.com/YYchainsAw/MothEffect/issues/48)、[#49](https://github.com/YYchainsAw/MothEffect/issues/49)、[#50](https://github.com/YYchainsAw/MothEffect/issues/50)、[#51](https://github.com/YYchainsAw/MothEffect/issues/51)，均 Open / In Progress、YYchainsAw / M5，元数据已回读。收尾提交 0e5370a / 4934627，隔离 UE 构建、114 Editor / 12 主地图 Game 回归通过，见 [验证报告](06_Test_Doc/Device_Refactor_Report_2026-10-14.md)；硬件 PIE、手感和独立包仍待验收；历史记录保留原验证范围，本轮未推送或创建 PR。

原工程补充验证：确认编辑器关闭后，Editor 常规构建 56.79 秒 Succeeded，新 DLL 的六项 Foundation 全通过；重新打开项目即可加载。该构建包含当前用户未提交控制器代码，未将这些改动纳入本轮提交。

2026-10-13 已按用户要求创建[PR #47](https://github.com/YYchainsAw/MothEffect/pull/47)：T18编辑入口、T19默认反馈与T20可替换原型外观，codex/t18-t20-prototype-feedback → main。base ce3a887、首个head 3a7d837，完整Git tree与已验证develop cadff96一致；126件差异，46件LFS对象11MB已上传，用户玩家资产未纳入。Assignee YYchainsAw，Labels feature/polish/area:ui/area:combat/area:art，Project MothEffect · 首版开发 / In Progress，Milestone M5 · 试玩与候选包验收，均已回读核对。首个head的Repository checks已Success（6s）；静态CI不代替游戏验收。使用Refs #20/#21/#22，关联Issue保持Open，各轮手动验收保留。本地继续develop；本轮PR沿用既有真实UE证据，随后T21独立包另行记录。 以下段落保留历史。

2026-10-13 T20原型完成：三种原创机关外壳、金属掩体模块、符文灯饰、共用材质与DA_PrototypeArt共17资产保存并新进程重载，重复运行文件SHA保持一致。机关与敌人背包共用可替换外观，主场地九块掩体与三灯饰由可关闭场景层接入；现有碰撞/导航和v0.18玩法基线保留。构建、107项全量Editor回归及最后一项夹具修正后的3项专项、实际D3D12/音频主地图12项通过；首轮围墙误识别、Static夹具、旧灰盒断言与导入失败如实留档。用户后续深度修改机制与整套UI，当前实现作为可运行基线；五组真实玩家/场地/独立包待验收，第三方缺依赖与完整恢复待补。实现3c6c8e5本地develop提交，#22 Open / In Progress，未推送/创建PR；配置见[Prototype_Art_Setup](05_Development_Guide/Prototype_Art_Setup.md)，实际证据见[T20报告](06_Test_Doc/T20_Report_2026-10-13.md)。以下段落保留历史。

2026-10-13 T19默认反馈已完成：枪口/确认命中/受伤、三机关启动、世界锁定方向、Active/Spent与过期警告使用无碰撞几何和8个原创短音效；DA_DefaultFeedback提供替换与参数入口。暂停随游戏时间，结算/重试同步清理，出生回调重入使用世代复核；缺失的HUD字体来源已修复，保留用户样式。构建、104项Editor回归及最终9项反馈/UI、真实D3D12/音频主地图11项通过，失败历史与五张原始截图见[T19报告](06_Test_Doc/T19_Report_2026-10-13.md)。四组用户PIE仍待验收，独立包留T21；#21 Open / In Progress，实现8112648、修正e00348f本地develop提交，未推送/创建PR。制作与验收见[Feedback_Setup](05_Development_Guide/Feedback_Setup.md)。以下段落保留历史。

2026-10-13 T18编辑接口已接入：保存Designer基础树和25个控件绑定，菜单文案/页面预览在Class Defaults，教学说明与布置在DA_MothTutorial；具体UI由用户制作。Editor构建、2项编辑回归、主地图10项及扩充UI2项通过，当前硬件PIE待验收。本轮本地develop提交，未创建新PR；原PR #46已合并。编辑入口见[UI_Setup](05_Development_Guide/UI_Setup.md)，结果见[T18报告](06_Test_Doc/T18_Report_2026-10-12.md)。以下段落保留历史。

2026-10-12 T18 PR记录：按用户要求已推送 codex/t18-hud-tutorial → main 并创建 [PR #46](https://github.com/YYchainsAw/MothEffect/pull/46)。创建期间核对到 T16/T17 PR #45 已合并至 main 8481463，因此 T18 直接面向 main；合并本身不扩大前轮手动验收范围。PR 的 Assignee 为 YYchainsAw，Labels 为 feature / area:ui，Project 为 MothEffect · 首版开发（Status In Progress），Milestone 为 M4 · 三波可玩闭环，均继承 #20 并在 GitHub 回读核对。使用 Refs #20，Issue 继续 Open / In Progress、六项完整验收框未勾选、五组当前硬件 PIE 待验收。实现阶段96项 Editor/主地图10项 Game 和最终三分辨率 UI 沿用已归档证据，本次 PR/元数据/文档未重复 UE；独立 PR 工作树297文件/50UE资产、44节阅读版、diff与LFS fsck通过。原开发与PR初始交付的完整 Git tree 相同，用户玩家蓝图未纳入。本地继续 develop；最终 head 的 Repository checks 另按实际 GitHub 运行核对。 以下段落保留历史。

2026-10-12 T18推进：原生 UMG HUD、三种原创矢量图标、实际动作驱动的三步教学和标题/暂停/教学完成/胜负/重开面板已接入。首次读取真实生命/弹药/持物/波次，事件更新并在换玩家/重开时解绑；进入战斗重载关卡，仅保存一次开始意图。教学分别核验爆炸死亡来源、真实物理抬升、投出且空中 W01 启动、D03 P01→D02 间接击杀；落地/错误启动补充，独立目标允许换顺序，暂停保留合法换弹。实现阶段96项 Editor 全量及主地图10项 Game 回归通过，后续教学重试/最终布局的 UI 分支在三种分辨率真实 D3D12 离屏渲染复测通过；既有资源/动画告警保留。#20 Open / In Progress，五组当前硬件输入与完整图形 PIE 待验收，TC26 外部盲玩及独立包留后续；本轮本地 develop 提交，未推送/创建 T18 PR，T16/T17 PR #45 待校验状态保留。 接入与验收见 [UI_Setup](05_Development_Guide/UI_Setup.md)，实测边界见 [T18报告](06_Test_Doc/T18_Report_2026-10-12.md)。 以下段落保留历史。

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
| 05_Development_Guide/ | [Device_Refactor_Setup.md](05_Development_Guide/Device_Refactor_Setup.md) | 当前道具基础重构、可编辑尺寸/延迟、水平射向、爆炸连锁及待执行验证 |
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
| 05_Development_Guide/ | [SoulCity_Arena_Setup.md](05_Development_Guide/SoulCity_Arena_Setup.md) | Soul City 独立工业竞技场、外观层编辑、依赖及实际验证范围 |
| 05_Development_Guide/ | [Niagara_Setup.md](05_Development_Guide/Niagara_Setup.md) | Niagara 战斗特效与调节 |
| 05_Development_Guide/ | [Slate_UI_Setup.md](05_Development_Guide/Slate_UI_Setup.md) | 当前 Slate / Kenney HUD、玩家与敌人生命条、菜单和用户准星保护；编辑入口及实际验证边界 |
| 05_Development_Guide/ | [Crosshair_Setup.md](05_Development_Guide/Crosshair_Setup.md) | 用户准星的命中/击杀事件与动画接口；原制作步骤保留历史，当前资产已制作并接入 Slate HUD |
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
