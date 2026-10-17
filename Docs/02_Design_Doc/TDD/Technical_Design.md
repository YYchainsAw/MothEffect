# Moth Effect（飞蛾效应）：技术设计

版本 v0.49 · 2026-10-17 · 游戏名：Moth Effect（飞蛾效应）

2026-10-17 道具触发更新：`ADeviceBase` 从 BeginPlay 起使用世界游戏时间计算出生保护，`TryActivate` 在保护结束前拒绝弹道和爆炸请求；保护不影响拾取/推力，放下/投掷不重置起点。D03 接纳时以当前装置位置按三维距离选择范围内最近有效存活 `AEnemyBase`，一次锁定指向 Actor 中心的三维射向；无目标沿用入射 XY 投影。具体参数、规则和新增验收见 [重构接入](../../05_Development_Guide/Device_Refactor_Setup.md)；以下历史验证段落仍只代表当时版本。

2026-10-14 贴图准星接口：`UHealthComponent` 在真实伤害与致死状态提交后，经 `UMothFeedbackSubsystem` 发出 `FMothEnemyHitFeedback`，快照包含命中 ID、目标 ID、实际伤害、剩余生命、致死标志和世界命中位置，不持有敌人引用。仅本地玩家归属的敌人伤害有效，原生死亡提交仍先于表现通知；回调返回复核生命周期世代，防止旧命中进入重置后的新会话。`UMothHUDWidget` 按拥有玩家筛选并向蓝图提供命中、重置、暂停和页面事件；单独关闭原生命中 X 时仍可保留受伤边框。表现由用户制作，详见 [自定义准星接入](../../05_Development_Guide/Crosshair_Setup.md)。本轮源码及九项专项测试已写入，按用户选择未编译、未运行 UE/Automation、未修改资产。

2026-10-14 道具基础重构：`refactor` 本地提交 `f6d6b2a`（Refs #48/#49/#50）与 `f91db81`（Refs #51/#48）接入地面固定/独立尺寸、D03 水平入射投影、共用激活延迟及 D02 爆炸激活。现行规则见 [道具与交互规则](../GDD/Device_Interaction_Rules.md)，字段和迁移见 [重构接入](../../05_Development_Guide/Device_Refactor_Setup.md)。代码收尾至 4934627，隔离 UE 构建及 114 Editor / 12 主地图 Game 回归通过；当前硬件 PIE 和独立包仍待验收，见 [本轮报告](../../06_Test_Doc/Device_Refactor_Report_2026-10-14.md)。旧记录保持当时验证范围。

原工程补充验证：确认编辑器关闭后，Editor 常规构建 56.79 秒 Succeeded，新 DLL 的六项 Foundation 全通过；重新打开项目即可加载。该构建包含当前用户未提交控制器代码，未将这些改动纳入本轮提交。

2026-10-13 T20：PrototypeArtProfile独立提供显示模型/材料/局部Transform；设备与背包直接共用解析入口，材质按语义槽名适配OBJ重排。开关保留原外观与用户替换；场景层仅主竞技场生成一次，识别T17标签/材质/尺寸，排除大围墙，关闭/销毁恢复原渲染。原掩体承担碰撞/导航，新网格与灯饰无玩法查询；资产软引用目录纳入Cook。接口与改造步骤见[Prototype_Art_Setup](../../05_Development_Guide/Prototype_Art_Setup.md)，实际验证见[T20报告](../../06_Test_Doc/T20_Report_2026-10-13.md)。用户后续机制/UI改造按新规则更新权威文档、配置与测试。

2026-10-13 T19：新增世界内FeedbackSubsystem、可编辑Profile、瞬时Visual Actor和DeviceFeedback组件。命中/效果从提交后的原生入口触发；真实几何无碰撞/Overlap/导航/阴影，箭头使用缓存世界方向。声音、网格效果与屏幕Alpha按游戏时间暂停；生命周期关闭/重置先清记录再结束实例，出生回调返回复核玩法与表现世代。枪口、命中、三类型启动和过期声音均有默认资产；独立包资产纳入AlwaysCook，但本轮不声称完成打包。接口见[默认配置](../../../Source/MothEffect/Public/Feedback/MothFeedbackProfile.h)、[世界反馈](../../../Source/MothEffect/Public/Feedback/MothFeedbackSubsystem.h)，制作与替换见[Feedback_Setup](../../05_Development_Guide/Feedback_Setup.md)，结果见[T19报告](../../06_Test_Doc/T19_Report_2026-10-13.md)。

2026-10-12 T18 PR记录：按用户要求已推送 codex/t18-hud-tutorial → main 并创建 [PR #46](https://github.com/YYchainsAw/MothEffect/pull/46)。创建期间核对到 T16/T17 PR #45 已合并至 main 8481463，因此 T18 直接面向 main；合并本身不扩大前轮手动验收范围。PR 的 Assignee 为 YYchainsAw，Labels 为 feature / area:ui，Project 为 MothEffect · 首版开发（Status In Progress），Milestone 为 M4 · 三波可玩闭环，均继承 #20 并在 GitHub 回读核对。使用 Refs #20，Issue 继续 Open / In Progress、六项完整验收框未勾选、五组当前硬件 PIE 待验收。实现阶段96项 Editor/主地图10项 Game 和最终三分辨率 UI 沿用已归档证据，本次 PR/元数据/文档未重复 UE；独立 PR 工作树297文件/50UE资产、44节阅读版、diff与LFS fsck通过。原开发与PR初始交付的完整 Git tree 相同，用户玩家蓝图未纳入。本地继续 develop；最终 head 的 Repository checks 另按实际 GitHub 运行核对。 以下段落保留历史。

2026-10-12 T18推进：原生 UMG HUD、三种原创矢量图标、实际动作驱动的三步教学和标题/暂停/教学完成/胜负/重开面板已接入。首次读取真实生命/弹药/持物/波次，事件更新并在换玩家/重开时解绑；进入战斗重载关卡，仅保存一次开始意图。教学分别核验爆炸死亡来源、真实物理抬升、投出且空中 W01 启动、D03 P01→D02 间接击杀；落地/错误启动补充，独立目标允许换顺序，暂停保留合法换弹。实现阶段96项 Editor 全量及主地图10项 Game 回归通过，后续教学重试/最终布局的 UI 分支在三种分辨率真实 D3D12 离屏渲染复测通过；既有资源/动画告警保留。#20 Open / In Progress，五组当前硬件输入与完整图形 PIE 待验收，TC26 外部盲玩及独立包留后续；本轮本地 develop 提交，未推送/创建 T18 PR，T16/T17 PR #45 待校验状态保留。 见 [UI_Setup](../../05_Development_Guide/UI_Setup.md) 与 [T18报告](../../06_Test_Doc/T18_Report_2026-10-12.md)。 以下段落保留历史。

2026-10-12 T16/T17 PR记录：按用户要求“创建pr”“保持待校验”，已推送codex/t16-t17-wave-arena → main并创建[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45)。main尚无T16，因此同一PR交付三波/胜负/重开及T17主竞技场，共51件功能/证据改动、11件新LFS资产；使用Refs #18/#19，两项Issue继续Open / In Progress，四组图形PIE分别待校验、完整验收框未勾选。原工程T16最终95项Editor/5项Game及T17主场地8项Game沿用既有真实记录，本次仅PR与文档，不重复执行UE。独立PR工作树验证271件文件/49件UE资产、42节阅读版、diff及LFS fsck通过；11件LFS对象220KB已上传，用户玩家蓝图未纳入。本地继续develop，GitHub Repository checks按最终head另行核对，静态CI不代替游戏验收。以下段落保留历史。

2026-10-12 合并与T17推进：PR #42/#43/#44已合并到main，本地develop以cbe4330同步origin/main 0780180，保留用户玩家蓝图。L_Arena_01已落地60×50米三区、六低/三高掩体、双侧120cm平台坡道及南北回转通路；八点出生沿用T16，默认启动改为主竞技场，Ready由MothStartEncounter开始，完整UI继续T18。八件新LFS资产保存重载后15条路径完整，原工程Editor构建27.15s Succeeded，真实主地图8项Game全部Success，errors=0 / warnings=156，新增三项场地测试无告警；既有Paragon/Murdock资源告警保留。实现d9b3851、测试dce5cc6已本地提交，T17 #19 Open / In Progress，四组图形PIE待验收，战斗参数v0.17未变。本地T16依赖仍未推送/创建PR；此次合并不扩大先前手动验收范围。以下段落保留历史。 保存布局源见[T17_Arena_Layout](../../../SourceArt/Levels/T17_Arena_Layout.json)，接入见[Arena_Setup](../../05_Development_Guide/Arena_Setup.md)。

2026-10-12 T16推进：三波导演已接入4 / 3+3 / 4+4固定种类/出生点，准备与延迟批次均计入待生成；真实胶囊/静态导航/安全距离检查和完整预警，占位后只在原点重试。玩家死亡/界外判负，末波结束胜利，World-local开关先关闭伤害/外力/生成再清对象与任务/绑定；当前地图OpenLevel重开，暂停结果页可重开，跨波不刷新Dormant闲置。保存BP_WaveDirector、原创警戒材质和L_T16Flow三件资产及八点静态导航，参数v0.17/Config和DEC35同步。Editor构建、最终全量95项Editor、5项真实Game流程与2项Game导航Success；实际4/6/8出生及8存活峰值、批次空窗、物理预警中断、界外、跨波闲置和十次全新World重开有证据。最终Editor导出errors=0 / warnings=1（引擎联网探测超时），此前94项warnings=0；Game流程errors=0，既有Paragon/Murdock加载/动画等156条warnings如实留存。#18保持Open / In Progress，当前图形PIE待验收；T14/T15仍待验收，T17最终场地、T18完整UI/教程及独立包/G3继续对应任务。本轮develop本地提交7d4edba/8932848/dca4554，未推送/创建T16 PR，用户玩家资产未纳入。以下段落保留历史。 类接口见[导演源码](../../../Source/MothEffect/Public/Framework/MothWaveDirector.h)，接入见[Wave_Flow_Setup](../../05_Development_Guide/Wave_Flow_Setup.md)。

2026-10-12 T15 PR记录：用户要求“先保持待验收 创建pr”，已推送codex/t15-enemy-drops并创建[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43)，当前Open。T14 PR #42尚未合并，T15暂以codex/t14-shooting-ai为目标，只显示T15的36件改动，待#42合并后再改为main。#17继续Open / In Progress，当前图形PIE待验收、验收框保留未勾选；不关闭任务。原工程Editor构建、88项全量与最终11项复测/2项Game导航沿用已归档实测，本次仅PR与文档记录，未重复运行UE。本地继续develop，玩家原有资产改动未纳入。现有workflow仅自动检查main/develop目标，叠加PR按最终head手动运行Repository checks，实际结果另以GitHub检查页为准。以下段落保留历史。

2026-10-12 T15推进：三个BP_EnemyBase子类以青色三角棱柱D01、橙球D02、紫长条D03预告固定掉落，共用T14射击AI；生命死亡后只尝试一次，近处有效出生、真实物理Dormant、24额度回收/保护与回调世代取消统一接入。保存12件新资产与L_T15Drops静态导航，参数v0.16/Config和DEC34同步。原工程Editor构建通过，全量88项Editor Success（新增11/既有77）；仅测试夹具修正后新增11项再次Success、2项Game保存导航Success，导出errors/warnings=0。T15 #17保持Open / In Progress、当前Murdock图形PIE待验收；T14 #16仍待用户PIE，PR #42未追加本轮内容。本轮develop本地提交，无T15推送/PR。以下段落保留历史。 死亡快照/有限球形出生搜索、容量回调后二次净空验证与旧世代取消见DEC34及[接入](../../05_Development_Guide/Enemy_Drops_Setup.md)。

2026-10-11 T14推进：EnemyBase一套射击AI已接入视线/完整预警/共用P01、最后可见位置追踪、静态导航与停滞持续换点、共享生命/推力及幂等死亡。参数v0.15与Config同步，保存BP_EnemyBase和带掩体/平台/已构建NavMesh的L_T14AI。原工程Editor构建通过，全量77项Editor Automation Success（新增11/既有66），另一个实际加载保存场地的Game进程2项导航Success；当前Murdock图形PIE待验收，#16为Open / In Progress。类/接入见[Enemy_Setup](../../05_Development_Guide/Enemy_Setup.md)，边界DEC33；T15固定掉落、T16波次/敌人注销与整局、T17最终场地未实现。以下段落保留历史。

2026-10-11 T13 接入：UDeviceLifecycleSubsystem 按 Game/PIE World 统一容量，SpawnDevice 先预占总额，BeginPlay 兜底登记，Active 按启动序安全替换，P01 在初始化/首次消耗/销毁登记释放；状态提交与名额预占早于外部回调，重置通过世代与禁入防止遗留。Dormant 按游戏时间分段累计，Held/暂停/玩法禁用冻结，放下/投掷继续，末5s文字警告，Spent立即出数后残留0.25s；显式安全盒或开启的世界边界执行清理。配置副本见 DefaultGame.ini，设计边界 DEC32，接入见 [Lifecycle_Setup](../../05_Development_Guide/Lifecycle_Setup.md)。用户关闭编辑器后修正Unity测试函数重名，原工程Editor正式构建/链接与新DLL的全量66项Automation通过；用户在五项清单后确认“测试通过 创建pr”，当前角色PIE按用户报告通过；完整整局和正式敌人流程继续随后续任务验收。

2026-10-11 T12 验证：复用 P01 和现有 D01–D03，十项新增集成通过真实 World::Tick / Chaos / ProjectileMovement Sweep / Timer / 帧末推力完成直接激活、两类外力移入弹道及布局阴性对照；最终 Editor 构建与全量 53 项 Automation 通过。P01 可选日志关联 spawn/impact/expired 的 HitId、来源、目标和接受结果，D03 按实例日志开关传递。保存 L_T12Chains 普通装置测试房、核对重载配置，保存 Blueprint 另行实际飞行命中；参数不变，DEC31 不允许直接激活后续目标或在连锁中传送对象。接入见 [Natural_Chain_Setup](../../05_Development_Guide/Natural_Chain_Setup.md)，完整范围见 [T12 报告](../../06_Test_Doc/T12_Report_2026-10-11.md)；用户已确认五站连锁/布局对照及暂停/清理的当前角色 PIE 通过，#14 为 Closed / Done；正式敌人和完整 G2 随后续任务验收。

2026-10-11 T11 完成：DeviceBase 的 Launcher 分派和 ALaunchColumn 已落地；通过 JamGroundTrace / WorldStatic 查询承托标签、法线及最终方形体积净空后生成世界竖直柱。伸出时种子和实体关闭碰撞，升柱查询逐 Actor 单次弹射，复用帧末 Impulse 合并；完全伸出后等待动态占用净空再开实体，收回先关查询/阻挡并解绑实际站立角色、恢复 Falling；禁用/种子 EndPlay 双向清理。两项灰盒、Editor 构建与 43 项 Automation 已通过，用户在碰撞排查后确认当前 PIE 通过，#13 为 Closed / Done。15:19 日志记录三次成功生成及正常结束，包含空中启动。测试地面须为 WorldStatic（BlockAll 或等效自定义响应），BlockAllDynamic 的 WorldDynamic 类型会被探地过滤；标签不能代替对象类型。入口见 [Launcher_Setup](../../05_Development_Guide/Launcher_Setup.md) 和 [T11 报告](../../06_Test_Doc/T11_Report_2026-10-11.md)，完整自然连锁和敌人接入仍随后续任务验收。以下旧实现快照保留为历史。

2026-10-11 T10 完成：DeviceBase 的 Bomb 分派与 IImpulseReceiver / ExternalImpulseComponent 已落地。爆心在 Active 通知前固定；球查询按 Actor 去重、以胶囊/物理根中心判半径，伤害与推力共享 WorldStatic 实体遮挡结果；不调用其他装置的弹道接口。外力在所属 World 的 OnWorldPostActorTick 合并、替换柱子 Z 并统一限速，暂停保留，死亡/禁用/EndPlay 清理。已保存 BP_DeviceBomb，Editor 构建及 32 项 Automation 通过；用户随后确认当前 PIE“测试通过”，#12 为 Closed / Done。正式敌人/D01 组合及完整连锁随对应阶段接入，详情见 [Bomb_Setup](../../05_Development_Guide/Bomb_Setup.md) 与 [T10 报告](../../06_Test_Doc/T10_Report_2026-10-11.md)。以下旧实现快照保留为历史。

2026-10-11 T09 收尾：失焦取消不再只依赖 Controller Flush，角色 Tick 与射击许可共同检查应用焦点；释放门控仍读取真实左键，不因 Canceled/Flush 自动放行。UnPossessed 停连射并取消换弹/恢复和 Held，旧角色不留下延迟任务。原生测试分别替代硬件按键与焦点状态，使用真实隔离 World::Tick 验证暂停；Fire/Reload/Throw 的不可播放 Montage 不参与玩法结算。Editor 构建与 23 项 Automation 已通过，用户随后确认当前真实角色 PIE 通过，T09 已关闭；正式胜负/重开联调随 T16 补测，见 [T09 报告](../../06_Test_Doc/T09_Report_2026-10-11.md)。

2026-10-08 方向表现接入：用户反馈游戏内没有方向提示。原生 UArrowComponent 绘制依赖 BillboardSprites 等视图设置，不能将它作为唯一游戏内提示；BP_DeviceEmitter 在绝对旋转的 EmitterDirectionMarker 下添加无碰撞 DirectionVisual 锥体网格，局部 Y/Pitch=-90° 令锥尖指向父组件 +X，现有原生显隐传播与方向锁定不变。步骤见 [Emitter_Setup 第 4 节](../../05_Development_Guide/Emitter_Setup.md#4-创建-bp_deviceemitter-与方向提示)，用户随后确认 PIE 方向显示通过；最终蓝图配置存盘、完整 TC03–TC05 和 G1 仍待核对。本轮不改玩法源码。

2026-10-08 17:06 T08 复测：用户 Devices 九项与 Projectiles 三项全部 Success，`23eb829` 的 UWorld::EndPlay 收尾修正已复测，最新轮次没有再出现缺少 EndPlay 警告。证据见 [T08 验证记录](../../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)；真实物理、方向表现、输入与空中成功率仍待 PIE/G1，助手未运行 UE。

2026-10-08 T08 测试收尾：用户 Devices 九项 Automation 均通过，但三个 Emitter 案例带 World 清理警告。隔离测试 World 已在 `23eb829` 改为调用 UWorld::EndPlay 后 DestroyWorld，让 Actor/Subsystem 收尾并清除 BegunPlay 标志；修正待编译/复测。P01 三项与真实 PIE 仍待结果，原始日志见 [T08 验证记录](../../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。

2026-10-08 T08 源码交付：`ARuleProjectile` 接入 Sphere Sweep、零重力直线移动、唯一 HitId、首次阻挡先关闭碰撞再调用 Ballistic 接口与独立寿命；仅忽略生成者，不给激活来源角色免伤。`ADeviceBase` 按 Emitter 分派 D03，首次命中锁定世界方向；方向箭头采用绝对世界旋转，出生点随物理机身平移；世界 Timer 控制启动延迟、固定发射时刻与到期，出生静态受阻消耗该发，Spent 停止任务并短暂残留后销毁。新增三项 P01、三项 D03 Automation 源码，尚未编译或运行；接入与 TC03/TC04/TC05 待执行步骤见 [Emitter_Setup](../../05_Development_Guide/Emitter_Setup.md)。T07 边界、T09 完整输入、T12 三机关连锁、T13 容量/闲置过期与 T16 全局清理继续按原任务验收。

2026-10-08 T07 实施：Character 接入 E 候选拾取/安全放下、Held 单次投掷、持物挂点/隐藏枪、世界 Timer 投掷恢复、换弹成功拾取取消及生命周期清理。装置拥有者提交先完成附着/释放与角色引用，再通知状态；安全释放按实际物理根球体和响应检查路径及端点。`ARifle::GetAimTarget` 供步枪/投掷共用相机目标。用户增量编译成功，新增三项释放空间 Automation 与既有三项状态测试均通过；PIE 仅基础拾取/放下/投掷确认通过，其余边界与表现待实测，见 [T07 接入](../../05_Development_Guide/Device_Interaction_Setup.md#8-2026-10-08-已验证范围)。D03/P01 与完整输入回归继续属于 T08/T09，T07 尚未全项验收。

2026-10-07 实施更新：用户报告 T05 上半身动画及当前中断/换弹按压/死亡/暂停/失焦分支 PIE 通过；[PR #34](https://github.com/YYchainsAw/MothEffect/pull/34) 已合并，T05 为 Closed / Done。T06 新增 `DeviceTypes` 与 `ADeviceBase`，复用已有命中契约，提供物理根/射击球、先提交 Active 再执行效果、状态事件与 Spent/Destroyed 清理；Held 提供拥有者提交/释放的内部入口，实际拾取输入、附着与安全释放属于 T07。D01/D02/D03 原生效果及容量/过期尚未实现。T06 为 Open / In Progress，助手未编译或运行，用户接入步骤见 [Device_Setup](../../05_Development_Guide/Device_Setup.md)。

用户已确认 UE 5.8、蓝图与 C++ 混合开发、正式显示名 Moth Effect（飞蛾效应）、魔法朋克风格，以及 v0.9 的玩家方案：C++ 玩法状态机、AnimBP 移动状态机、上半身 Montage、复用本地模板 Rifle 动画，首版不使用 GAS。当前工程文件为 [MothEffect.uproject](../../../MothEffect.uproject)。v0.10 已写入 T04 相机/转向/瞄准冲刺的 C++ 基础，见 [玩家第一步接入](../../05_Development_Guide/Player_Setup.md)；用户负责 UE 资产配置与试玩，并明确要求助手不代为编译。2026-10-06 的任务状态和记录情况见 [开发计划](../../05_Development_Guide/Development_Plan.md)；完整玩家系统与包体仍须对应测试证据。玩法以 [道具与交互规则](../GDD/Device_Interaction_Rules.md) 为准，数值只维护在 [玩法参数基线](../GDD/Gameplay_Parameters.json)。

v0.12 历史交付：用户报告移动/瞄准动画成功；T05 写入 ARifle、UHealthComponent、FHitContext、BallisticReactive、玩家输入门控与 Ready/Reloading/Dead，交付时助手未编译/运行。2026-10-06 同步：T05 基础接入、射击、换弹与生命项已勾选，上半身 Montage 和适用测试记录待补，见 [Weapon_Setup](../../05_Development_Guide/Weapon_Setup.md)。Carrying/ThrowRecovery 仅保留枚举，机关与整局任务仍为 Todo。移动速度的已知代码/账本差异继续在 Player_Setup 记录，本次不调整数值。

v0.13：用户希望在角色蓝图预览并调整完整武器，Character 新增 Mesh 下的 UChildActorComponent **RifleComponent**，通过 WeaponSocket 挂接 BP_Rifle。RifleClass/RifleAttachSocket 控制类型与挂点，组件 Transform 控制偏移；OnConstruction 同步类型/挂点但不重设偏移，仅在类实际变化时设置 ChildActorClass。BeginPlay 获取现有子 Actor 并绑定事件，EndPlay 解绑并取消任务，由组件负责销毁；旧 SpawnActor 路径和 RifleRelativeTransform 字段移除，已有偏移需按接入文档手动迁移。当次交付仅复核源码；2026-10-06 的 T05 Issue 已勾选组件预览与运行单武器项，相关构建与证据待补。

## 1. 起步与职责

建议沿用现有 Windows C++ Third Person 工程，以模板角色、移动动画与输入作为起点。空白工程适用于需要从零定制角色框架或已有基础资源的情况；本作单人周期内，模板能省去重新搭建这些基础功能的工作。用基础几何体表现枪、机关和敌人，先验证空中触发，再补持枪/投掷表现；模板本身不代表本作越肩射击系统已经接入。[Epic：Third Person 模板](https://dev.epicgames.com/documentation/unreal-engine/third-person-template-in-unreal-engine)

已存在的模板源码、资产、模块依赖与配置核对记录见 [UE 工程配置](../../04_Engine_Config/UE_Project_Configuration.md)；类与蓝图映射沿用下表。


C++ 管状态、命中、伤害、推力、定时器和容量；蓝图管组件组装、相机参数、动画混合、魔法朋克材质、音效、特效、界面和关卡引用。蓝图调用 C++ 入口，不另写一套伤害或激活流程。继续使用已有运行时模块 `MothEffect`，不新建 `Jam` 模块；首版不使用 GAS 已由用户确认，Mass、自定义插件与联网框架仍不进入首版。GAS 取舍见第 3.6 节。

| C++ 类 / 父类 | 蓝图资产 | 责任 |
|---|---|---|
| `AMothEffectCharacter : ACharacter` | `BP_ThirdPersonCharacter` | 移动/转向、越肩相机、玩家行动状态、拾取/投掷、输入门控、玩家受推力 |
| `AMothEffectPlayerController : APlayerController` | `BP_ThirdPersonPlayerController` | 输入映射、暂停、HUD、胜负界面 |
| `AMothEffectGameMode : AGameModeBase` | `BP_ThirdPersonGameMode` | 创建玩家、胜负、重开入口 |
| `AMothEffectGameState : AGameStateBase`（新增计划） | `BP_MothEffectGameState`（新增计划） | 当前波次/局内状态及 UI 通知；无复制需求 |
| `ARifle : AActor` | `BP_Rifle` | W01 双段检测、射速、弹匣、换弹 |
| `ADeviceBase : AActor` | `BP_DeviceBase` | 共用状态机、物理根、激活分派、过期/回收 |
| 同上，通过 `EDeviceKind` 分派 | `BP_DeviceLauncher / BP_DeviceBomb / BP_DeviceEmitter` | 分别绑定 D01/D02/D03 数据与外观；不增加三个 C++ 子类 |
| `ALaunchColumn : AActor` | `BP_LaunchColumn` | 升柱表现、上升路径检测、一次弹射、净空后实体支撑 |
| `ARuleProjectile : AActor` | `BP_RuleProjectile` | 敌人与机关共用 P01，移动、命中、寿命 |
| `AEnemyBase : ACharacter` | `BP_EnemyBase` | T14射击AI/预警/P01/死亡已实现，内建AIController与静态NavMesh；固定掉落待T15 |
| `UDeviceLifecycleSubsystem : UTickableWorldSubsystem` | 自动创建，无场景 Actor | 总道具/同类 Active/P01 容量、累计闲置、界外与重置；SpawnDevice/ResetManagedObjects 供掉落/整局接入 |
| `AWaveDirector : AActor` | `BP_WaveDirector` | 波次、生成点、存活敌人；机关容量交给WorldSubsystem |
| `UHealthComponent : UActorComponent` | 角色上的组件 | HP、一次死亡事件 |
| `UAnimInstance`（UE 内建父类，先不新增 C++ 动画类） | `ABP_MothPlayer`（用户已创建） | 读取玩家与移动组件快照，执行移动状态机、AimOffset、上半身分层；不修改玩法状态 |
| `UDeviceDefinition : UDataAsset` | `DA_Device_D01 / D02 / D03` | `FDeviceConfig` 与对应 BP 类/资源引用 |

拾取、放下、投掷先保留在 Character 的独立方法中；复杂度增加后再抽成交互组件，并同步本表。枪械只由 ARifle 维护弹匣、射速和换弹任务，HealthComponent 只维护一份血量，Character 只维护一份 Held 引用，不额外创建重复持有这些数据的 WeaponComponent 或背包系统。

## 2. 数据与调用契约

以下名称、签名为**本项目自定义 API 契约**，不是 UE 内建功能。玩家行动/左键枚举、FHitContext、BallisticReactive、ARifle、HealthComponent 及相关事件已写入原生源码，T05 当前 PIE 分支由用户报告通过。T06 的 EDeviceState/EDeviceKind、ADeviceBase::TryActivate 与 OnDeviceStateChanged 已实现，用户当前状态壳测试通过；T07 的 TryPickup、TryReleaseHeldDevice 已实现，Automation 与基础 PIE 通过，适用边界待验收。D01/D02/D03 效果、推力、P01和T13容量/闲置/清理已有原生验证，当前角色PIE按对应任务记录；波次/正式整局等后续入口仍为计划接口。

- `EDeviceState`：`Dormant, Held, Active, Spent, Destroyed`；`Destroyed` 是清理末态，可不留存于已销毁对象。
- `EDeviceKind`：`Launcher, Bomb, Emitter`。
- `EPlayerActionState`：`Ready, Carrying, ThrowRecovery, Reloading, Dead`；由 Character 唯一持有。`Ready` 表示持枪可接收动作请求，实际开火还需弹药、射速、按压与局内许可检查。
- `EPrimaryPressMode`：`None, Fire, Throw, Blocked`；记录本次左键按压归属。它是输入会话状态，不替代玩家行动状态。
- `FHitContext`（蓝图显示名 `ST_HitContext`）：`FGuid HitId`、`AActor* SourceActor`、`APawn* InstigatorPawn`、`FVector ImpactPoint`、`FVector ShotDirection`、`float Damage`。方向为归一化世界飞行方向，不用表面法线；引用仅在命中处理期间使用并验证有效性。
- `FDeviceConfig`（蓝图显示名 `ST_DeviceConfig`）仍为规划数据契约；当前机关直接使用 `ADeviceBase` 可编辑属性，不把本轮重构写成该 DataAsset 接口已经落地。字段与 [玩法参数基线](../GDD/Gameplay_Parameters.json) 对应，单位为 cm、秒、cm/s。共用激活/运动/尺寸在 device 组，ShotRadiusCm 沿用 throw.deviceShotColliderRadiusCm，伤害与容量分别来自 weapon/projectile/limits 组。

| 自定义入口 / 事件签名 | 调用约定 |
|---|---|
| `UBallisticReactive / IBallisticReactive`：`bool ReceiveBallisticHit(const FHitContext& Context)` | 角色转交 Health，机关转交 TryActivate；返回 true 只代表规则被接受 |
| `UImpulseReceiver / IImpulseReceiver`：`void ReceiveImpulse(FVector Velocity, bool bOverrideZ, FGuid EventId)` | 当帧缓存，EventId 去重；帧末先合并爆炸增量、再用柱子目标替换 Z、最后统一限速 |
| `ADeviceBase::bool TryActivate(const FHitContext& Context)` | 仅出生保护结束、Dormant 且容量许可可成功；先预占、保存首次上下文及一次发射方向并提交 Active，再通知、按 ActivationDelaySeconds 延迟一次效果；弹丸与爆炸请求共用入口 |
| `ADeviceBase::float SpawnActivationProtectionSeconds` / `float EmitterTargetRangeCm` | 可编辑非负出生保护时间和索敌半径；BeginPlay 游戏时刻只记录一次，暂停冻结，拾取/释放不重置；D03 接纳时按三维距离找最近存活 AEnemyBase 并锁定指向 Actor 中心的三维方向 |
| `ADeviceBase::bool IsActivationPending() const` / `float ActivationDelaySeconds` | 等待期属于 Active，不增加枚举；可编辑非负延迟，World Timer 暂停冻结，Finish/Destroy/禁用取消；零延迟为明确的同步专项覆盖 |
| `ADeviceBase::bool IsGrounded() const` / `bool WakeForExternalImpulse()` | Grounded 独立于生命周期；CanReceiveExternalImpulse 只允许 Dormant/Active D03，QueueImpulse 去重后先唤醒空中物理，再由帧末速度结算 |
| `ADeviceBase::void RefreshDeviceSize()` | BlueprintCallable/CallInEditor；更新 BodyRadiusCm/ShotRadiusCm 与独立 VisualScale，Shot 有效半径不小于 Body；运行时改属性后调用刷新 |
| `ADeviceBase::bool TryPickup(AMothEffectCharacter* Picker)` | 距离、遮挡、状态均满足才置 Held |
| `ADeviceBase::bool CanBePickedUp() const` | 已实现机关端资格查询；只允许未暂停、玩法许可的 Dormant，不代替 T07 的完整拾取检查 |
| `ADeviceBase::bool CommitHeld(AMothEffectCharacter* NewHolder)` / `bool CommitReleased(AMothEffectCharacter* ReleasingHolder)` | 已实现的私有入口，仅 Character 在安全检查成功后调用；Held 必须有有效拥有者，只有该拥有者可释放；不对蓝图暴露任意状态写入 |
| `ADeviceBase::bool FinishActivation()` / `void DestroyDevice()` / `void SetGameplayEnabled(bool bEnabled)` | 已实现一次 Spent、最终销毁与玩法取消；终态不复活，取消不重复执行 StopEffect |
| `AMothEffectCharacter::bool TryReleaseHeldDevice(bool bThrow)` | 统一放下/投掷安全检查；失败保持 Held |
| `AMothEffectCharacter::bool TryPickupDevice()` | 验证候选；成功时统一提交 Held 引用/Carrying，并取消未完成换弹；失败不改变原行动 |
| `ARifle::bool TryStartFire()`；`void StopFire()` | Character 授权后请求首次射击与定时连射；每发重新检查许可、射速和弹药；停止操作幂等 |
| `ARifle::bool TryBeginReload()`；`void CancelReload()` | 有缺弹且允许换弹才启动任务；只有未取消任务计时完成才一次补弹；取消清 Timer 与任务标识 |
| `OnReloadFinished(bool bCompleted)`；`OnShotFired(ARifle* Rifle)` | Character 协调行动状态，蓝图更新动画/HUD；旧任务回调不得结束新任务，开火事件只在实际消耗一发时广播 |
| `OnPlayerActionStateChanged(EPlayerActionState OldState, EPlayerActionState NewState)` | C++ 完成行动转换后广播；AnimBP/HUD 读取，不回写状态 |
| `UHealthComponent::bool ApplyHit(const FHitContext& Context)` | HitId 去重；死者不再扣血；返回是否接受伤害 |
| `ARuleProjectile::bool InitializeProjectile(const FVector& WorldDirection, AActor* SourceActor, APawn* InstigatorPawn)` | deferred 出生到 FinishSpawning 之间初始化一次；归一化世界方向与出生 HitId，生成者避让，Pawn 只用于归因 |
| `ARuleProjectile::bool ProcessBlockingHit(const FHitResult& Hit)` | 首次真实阻挡先提交消耗并关闭碰撞/移动，再调用 Ballistic 接口；接受与拒绝都消耗；出生短段命中同样走该入口 |
| `ADeviceBase::int32 GetEmitterShotAttempts() const` / `int32 GetEmitterProjectilesSpawned() const` | D03 调试计数；受阻或配置/出生失败仍消耗当前时刻，但不增加出生计数；不代替 T13 全局容量登记 |
| `OnDeviceStateChanged(ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)` | C++ 修改状态后广播，蓝图只更新表现 |
| `OnDeviceStateChangedNative(ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)` | C++ 观察相同状态提交；回调可能继续结束/销毁，以 GetDeviceState 查询当前状态 |
| `OnHealthChanged(float CurrentHealth, float MaxHealth)`；`OnDied(AActor* Victim, AActor* SourceActor)` | 动态多播委托；每个角色 OnDied 最多一次 |
| `AWaveDirector::bool TryRegisterDevice(ADeviceBase* Device)`；`void UnregisterDevice(ADeviceBase* Device)` | 登记/注销均幂等，不能在 UI 中另行计数 |

**阻挡命中即消耗 P01，无论接口返回 true 或 false。** 墙体、Active D03 和已经启用实体碰撞的柱子阻挡弹丸；地面设备由 ShotCollider 查询球阻挡 P01，而实体根无碰撞。Held 关闭两球，弹丸可继续命中玩家。接口拒绝激活不返还弹丸，也不返还步枪弹药。D02 范围请求直接调用共用 TryActivate，普通物理碰撞不激活。

[玩法参数基线](../GDD/Gameplay_Parameters.json) 是设计数值权威，运行时从 DataAsset/默认配置读取；本版不承诺自动导入 JSON。首次录入与每次调参都核对 [玩法参数基线](../GDD/Gameplay_Parameters.json)，禁止在 Tick、关卡脚本或特效里藏伤害数值。

## 3. 越肩射击与输入

W01：①从相机沿准星射线找第一阻挡点；无命中则取射程终点。②从枪口向该点再检测，只有第二段实际命中才造成伤害/启动机关，`ShotDirection` 使用第二段方向。两段忽略玩家、枪及 Held 道具。先检测角色胸前到枪口的短段，枪口已被墙体阻挡或嵌入时取消射击伤害，提示“枪口受阻”，避免镜头看到目标却从墙另一侧开枪。SpringArm 防碰撞负责镜头，不能代替枪口检测。[Epic：单次射线返回首个阻挡命中](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Collision/LineTraceByChannel)

复用已有 /Game/Input 的 IMC_Default、IMC_MouseLook 与 IA_Move/Look/MouseLook/Jump，在现有控制器与角色输入绑定上扩展。新增 IMC_MothGameplay 只映射 IA_Primary/Interact/Aim/Reload；旧 IA_Sprint 不再由 C++ 绑定，不重复建立第二套移动与跳跃输入；新增输入资产沿用户实际目录，Action 放 /Game/MothEffect/Input/Actions，Mapping Context 放 /Game/MothEffect/Input/Mapping。`IA_Primary` 用简单 Boolean，不叠加 Hold/Release 触发器；单次交互与换弹绑定 Started，移动/视角轴使用 Triggered。左键一次按压流程与取消语义见第 3.3 节。[Epic：Enhanced Input 事件](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)

E 在空手时拾取准星附近的最近合格 Dormant，道具到角色需无遮挡；Held 时安全放下，Held 时 R 换弹不生效。拾取关物理、关碰撞并附着手部挂点；投掷先从角色外侧到释放点做与道具体积一致的 Sweep，再检查终点重叠，不能把释放点直接放到摄像机位置。路径阻挡或终点不合法则保持 Held 并提示。Detach 后切回 Dormant、开启重力与物理，设置 [玩法参数基线](../GDD/Gameplay_Parameters.json) 的初速度；可对物理根启用 CCD 并验证薄墙。放下同样检查空位，反复拾取放下不刷新累计闲置寿命。

### 3.1 玩家状态与转换

行动、移动和瞄准分别表达。移动由 CharacterMovement 的 MovementMode/IsFalling 与速度读取；瞄准记录输入意图和实际是否生效；开火是在 Ready 中按射速执行的动作，不创建独占 Firing 状态。不把空中、瞄准、携带的所有组合展开成一个大枚举，也不为玩家另建 StateTree。

| 行动状态 | 进入条件与权威数据 | 可执行行为 | 退出与失败处理 |
|---|---|---|---|
| Ready | 存活、无持物、无换弹或投掷恢复任务 | 移动/跳跃、瞄准；左键申请射击，E 拾取，R 手动换弹 | 拾取成功到 Carrying；有效换弹到 Reloading；请求失败保持 Ready |
| Carrying | 有一个有效 Held 引用，所属机关也为 Held | 移动/跳跃、瞄准投掷方向；E 放下，左键单次投掷 | 安全放下到 Ready；投掷成功到 ThrowRecovery；阻挡保持 Carrying；不响应 R 或枪械射击 |
| ThrowRecovery | 物理释放已提交，Held 引用已清空，恢复计时进行中 | 移动/跳跃、瞄准；不接受新的射击/拾取/换弹请求 | 恢复计时到期到 Ready；提前左键为 Blocked，不缓存开火；放下不经过本状态 |
| Reloading | ARifle 已接受有效换弹请求 | 移动/跳跃、瞄准；E 尝试拾取；不接受射击或重复换弹 | 计时完成到 Ready 并补弹；拾取成功取消任务到 Carrying；拾取失败继续；普通受伤不取消 |
| Dead | HealthComponent 的一次死亡事件 | 禁止玩法输入；播放死亡/显示结果 | 停止开火、取消换弹与恢复任务，Held 随本局清理；重开创建新局，不原地返回 Ready |

~~~text
Ready --拾取成功--> Carrying --安全放下--> Ready
Carrying --投掷成功--> ThrowRecovery --恢复计时结束--> Ready
Ready --有效换弹--> Reloading --完成或取消--> Ready
Reloading --拾取成功，取消未完成换弹--> Carrying
任意存活行动 --死亡--> Dead
~~~

恢复计时结束只解除行动限制，不解除本次左键归属；是否可射击还要通过第 3.3 节。Held 对象意外销毁时验证引用并清理携带表现，存活角色回到 Ready；不得生成补偿复制品。正常容量/过期策略仍不得销毁 Held。局内暂停/结算是全局许可，不伪装成玩家死亡状态。

### 3.2 移动、朝向与相机

玩家持枪与携带时沿控制器/镜头的水平 Yaw 转向，WASD 按镜头水平轴移动；角色本体不跟随镜头 Pitch/Roll 倾斜。起步使用 `bUseControllerRotationYaw=true`、`bOrientRotationToMovement=false`，替换模板的沿移动方向转身方式；AnimBP 使用角色局部水平速度组成八方向移动。镜头俯仰交给 AimOffset 与枪械瞄准方向。

2026-10-08 用户将移动改为单 Jog。`JogSpeed` 是正常、瞄准、携带的唯一最大水平速度，原生默认值与 `player.jogSpeedCmPerSec` 同步；移除 WalkSpeed/AimMoveSpeed/SprintSpeed 分支，旧蓝图保存的三个速度不能覆盖新字段。C++ 不再绑定 SprintAction，旧 DoSprintStart/End/CancelSprintUntilRelease 只作兼容入口，IsSprinting 始终为 false。跳跃仍由 CharacterMovement 控制，按新 TC34 验收。

空中控制使用 `player.airControl`，起步复用本地源码默认值；受柱子/爆炸推动后不进入眩晕状态，仍能转镜头、瞄准和在行动许可下射击。跳跃只在 CharacterMovement 允许时接受，不增加二段跳；落地不强制锁输入，首版无坠落伤害。相机臂长、越肩偏移、FOV/混合时间读取 camera 组，投掷与射击共用准星视线目标；相机碰撞不代替释放体积或枪口检测。

### 3.3 左键、换弹与中断

1. Started 先检查全局许可和 `bRequirePrimaryRelease`。Carrying 将本次归属设为 Throw，尝试一次安全投掷；失败也保持 Throw 到实际松键。Ready 且允许射击时设为 Fire，由 ARifle 执行首发并用单一 Timer 连射；其他情况设为 Blocked。Primary 的 Triggered 不再独立开枪，避免同次按下重复首发。
2. ARifle 每发检查玩家仍为 Ready、本次归属仍为 Fire、局内许可、射速和弹药。Completed 停止连射并在确认物理松键后清本次归属；Canceled 停连射、置 Blocked 并设置 `bRequirePrimaryRelease`，实际松键后才清门控，不能把输入上下文移除造成的取消当作玩家已松键。
3. 成功投掷清 Held、恢复持枪表现、进入 ThrowRecovery，计时长度只读取 `throw.minGunRecoverySeconds`。同次长按始终不能开枪；松键与恢复计时两个条件顺序不限，但都完成后再出现新的 Started 才能射击。恢复期间再次按下只显示恢复提示，不在计时到期时自动开枪，必须再松键重新按；失败投掷不启动恢复计时。
4. 换弹时先停止连射；空匣不开枪且不自动换弹，R 才请求换弹，满匣拒绝无效请求。换弹计时只由 ARifle 持有；完成一次补弹，取消不补弹。拾取候选完整验证成功后统一取消未完成换弹并提交 Carrying，不能先取消再发现拾取失败。取消/死亡使任务标识失效，迟到回调不补弹或覆盖新行动。
5. 暂停冻结玩法与动画计时，保留合法换弹/恢复剩余时间，同时停连射、清瞄准/跳跃输入意图并设置松键门控。失焦也停连射并设置门控；Canceled 或输入 Flush 本身不能解除门控，重新获得焦点后检查实际按键已松开，之后才允许新 Started。死亡/胜负/EndPlay 取消玩法任务、停止攻击并解除无效引用/绑定；结算后的回调不得继续伤害或补弹。

### 3.4 动画资产与图结构

2026-10-08 当前玩家为用户创建的 BP_MothPlayer / ABP_MothMurdock，使用 ParagonMurdock 模型和配套动画。人物和枪一体时启用 bUseIntegratedWeaponMesh：RifleComponent 挂角色根组件，隐藏独立枪 Actor，RifleClass 留空可使用原生 ARifle；每发从角色 Mesh 的 IntegratedMuzzleSocket 获取世界枪口位置，不要求旧 WeaponSocket。缺失/销毁的枪口来源拒绝射击且不扣弹，不回退到独立枪或组件中心；相机目标、枪口遮挡与弹药/换弹规则保留。角色骨骼在一体模式始终刷新，隐藏逻辑枪不影响真实枪口读取。迁移及 Montage Slot 配置见 [Murdock_Setup](../../05_Development_Guide/Murdock_Setup.md)。下面模板资源与旧玩家记录属于迁移前方案，不能代替 Murdock 验收。

本地 `/Game/Characters/Mannequins/Anims/Rifle` 已静态核对存在 `MM_Rifle_Fire / Reload / Equip / DryFire`、`MF_Rifle_Idle_ADS`、八方向 Walk/Jog、Rifle 跳跃系列、HitReact 与 `AIM/AO_Rifle`。文件存在不等于骨架、Additive、Root Motion 或 Notify 已验收；在编辑器逐项核对后引用。先复用本地资产，不把模板 Combat 近战示例当作本作射击实现。

新增动画资产沿用户实际目录 `/Game/MothEffect/Animations/Player`：`ABP_MothPlayer` 位于该目录，`BS_MothRifleLocomotion` 位于 Combat 子目录，`AM_MothRifleFire / AM_MothRifleReload` 也归入 Combat；按实际需要增加携带姿态/短释放表现。用户报告当前移动/瞄准动画测试成功，完整分支待留证；角色仍使用现有 BP_ThirdPersonCharacter。移动/AO 节点见 [玩家输入与动画接入](../../05_Development_Guide/Player_Setup.md#5-连接-abp_mothplayer)，上半身 Slot/骨骼分层见 [步枪接入](../../05_Development_Guide/Weapon_Setup.md#5-再接入上半身射击与换弹)。

~~~text
Character / CharacterMovement / Rifle 的只读快照
  → 移动状态机 Grounded / JumpStart / InAir / Land
  → 持枪或携带基础姿态 + 持枪 AimOffset
  → UpperBody Slot + 骨骼分层混合
  → 存活姿态；死亡时由全身死亡表现覆盖
~~~

| 动画部分 | 选择与转换依据 | 必须保证 |
|---|---|---|
| Grounded | 局部水平速度的二维 BlendSpace；现有资产以等价的 Direction / GroundSpeed 表达，复用八方向 Jog 和待机 | 侧移/倒退姿态与真实移动方向一致 |
| JumpStart / InAir / Land | 主动跳跃可进 JumpStart；被弹射、爆炸推动或走落边缘直接进 InAir；竖直速度选择上升/顶点/下落，落地回 Grounded | 任意地面状态都可直接转 InAir；新外力可中断 Land；落地动画不锁玩法 |
| AimOffset | 存活持枪时根据镜头相对角色的瞄准方向混合，范围按 AO 资产能力核验 | 携带仍能调整镜头/投掷方向，但不用步枪 AO 强行扭曲携带姿态；换弹时按姿态需要降低 AO 权重 |
| Fire / Reload Montage | OnShotFired 与有效换弹事件请求上半身动画；UpperBody Slot 必须配合 Layered Blend per Bone/骨骼遮罩 | 不覆盖腿部移动；Reload 优先于 Fire，动作许可在 C++ 处理，同组 Montage 不互相误抢 |
| Carry / Release | 持物挂点、简化持物姿态及短释放反馈；携带时收枪到配置挂点或隐藏枪网格，离开携带恢复 | 释放反馈不能延长恢复计时；未有专用投掷动画不阻挡 G1 |
| Hit / Death | 普通受伤先用 HUD/声音/轻微叠加反馈；死亡用全身表现 | 普通受伤不引入硬直或打断换弹；死亡动画失败也立即停止玩法 |

AnimBP 从角色、移动组件、武器读取局部速度、竖直速度、IsFalling、有效瞄准、ActionState、Held 有效性和换弹进度；C++ 是行动与任务的权威。射击判定、物理释放、扣弹、补弹、恢复与死亡均不能只依赖 AnimNotify 或 Montage 播完。Notify 可同步音效/视觉；Montage 不播放、被打断或更换 AnimBP 时，合法玩法任务仍按计时完成或按取消规则结束，不遗留锁。首版角色位移由 CharacterMovement 控制，动画不以 Root Motion 驱动移动。

动画序列包装 Montage 时核对骨架、Slot/Group、Additive 和播放时长；换弹表现适配 weapon 组计时，枪口与持物挂点按实际骨骼配置。模板原资产路径保持现状；模板目录目前不在 Git 跟踪范围，新 ABP 的外部依赖按 [版本管理恢复约定](../../03_Code_Standard/Version_Control.md) 记录并在 G0/新机器恢复时核验，不把新 ABP 已入库等同于其依赖已齐全。[Epic：动画状态机](https://dev.epicgames.com/documentation/en-us/unreal-engine/state-machines-in-unreal-engine)；[Epic：Montage 与 Slot](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-montage-in-unreal-engine)

### 3.5 玩家实施顺序与验收

G0 首包 → T04 基础玩家/转向/移动状态机 → T05 枪械/换弹/生命与上半身动画 → T06–T09 持物/投掷恢复/D03 空中启动 → T10–T12 外力与连锁 → T14–T18 敌人掉落/三波/HUD/重开。早期只制作最低可读动画，不等 T20 美术整合才验证移动、射击或释放。玩家新增验收为 TC34–TC38，原 TC06/TC24/TC31 同步补齐；各功能按任务进度与实际测试分支分别验收；T04 基础项与 T05 前三项已勾选，机关和闭环任务仍为 Todo。

### 3.6 GAS 决策与扩展边界

首版不使用 GAS，原因是单枪、单机、三机关的主要复杂度在物理抓持、命中、输入门控与生命周期，已有 Character / Rifle / HealthComponent 足以承担；GAS 不替代这些逻辑。GAS 也适用于单机，它能管理能力、属性、费用、冷却、状态效果等；当前收益与接入维护成本的取舍是本项目判断，不是官方对性能或适用性的限制。[Epic：Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)

只有明确扩大为大量技能、Buff、装备属性或联机预测时才重新评估。保持动作入口、伤害接口、配置与通知清楚可降低迁移范围，但接入仍要重构属性与动作许可；不能让 HealthComponent 和 GAS AttributeSet 并行持有两份权威血量。当前不启用 GAS 插件、不添加相应模块，也不因已有空 Abilities 目录认定能力系统已实现。

候选机关继续在世界 Actor/共用接口上评估：牵引扩展推力处理，诱光扩展 AI 目标选择，折光扩展弹丸命中，静滞扩展物理约束与清理，换位扩展安全空间交换。它们不因本次玩家方案确认而进入首版，选定时按 [候选机关道具](../GDD/Device_Candidates.md) 补齐规则与测试；投掷轨迹预览仍为 P1，多枪、背包与成长树仍为 P2。

## 4. 物理与三种机关

机关使用 USphereComponent 的 PhysicsBody 根，另挂 QueryOnly ShotCollider 球；视觉不参与玩法碰撞。`IsGrounded` 是独立运动状态：承托成立时清线性/角速度、停止模拟并令根 NoCollision，ShotCollider 仍供 Visibility/P01 查询。支持移除后恢复下落，显式推力/弹射由 `WakeForExternalImpulse` 先恢复空中模拟，重新落地固定。投掷/起飞由 Chaos 运动，根忽略 Pawn；不同时用 ProjectileMovement 移动机关。Held/Spent/已消耗 D01 种子关闭两球。P01 仍用 Sphere 根 + ProjectileMovement 和零重力直线 Sweep。[Epic：ProjectileMovement 与物理模拟关系](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UProjectileMovementComponent)

`DeviceMotion.cpp` 以实际缩放后的根球作短距离支持 Sweep，查询阻挡 PhysicsBody 的 WorldStatic/WorldDynamic 支撑、向上法线及非初始穿透，排除角色、其他道具和纯查询体。运动承托不要求 `MothColumnSurface`；D01 起柱仍单独要求静态对象与该标签。`GroundSnapDistanceCm/GroundUpNormalMin` 对应 device 参数组。`VisualScale` 只调整外观，`BodyRadiusCm` 控制空中/安全释放体积，`ShotRadiusCm` 控制射击球；`RefreshDeviceSize` 在构造和运行时刷新并约束 Shot 不小于 Body。已有 BP 覆盖值不会因修改原生默认值自动消失。

T08 当前碰撞接入：`Config/DefaultEngine.ini` 新增 `JamProjectile` 对象通道（`ECC_GameTraceChannel1`，默认 Block），映射统一维护在 `MothCollisionChannels.h`。P01 QueryOnly 球忽略同类弹丸、Visibility、Camera，并通过 MoveIgnoreActors 忽略自己的生成者；装置物理根忽略 P01，由较大的 ShotCollider 阻挡 P01。Held/Spent/Destroyed 仍关闭两球。步枪继续使用既有 Visibility；完整 JamDevice/JamWeaponTrace/JamGroundTrace 矩阵后续任务逐步接入，不能只改一端通道。蓝图若覆盖响应，接入时应按当前矩阵核对。

当前激活调度：BeginPlay 保存世界游戏时刻，TryActivate 在出生保护内先拒绝请求，不提交 Active 或预占激活容量，结束后需新的一次有效触发；游戏暂停冻结保护，拾取/放下/投掷不刷新。接纳后立即 Active/预占容量、保存标量/向量证据与 Source/Instigator 弱引用；世界 Timer 延迟 `BeginActivationEffect`，到时再次复核状态/World-local许可，取当前有效弱引用并只开始一次效果。暂停冻结，FinishActivation/禁用/Destroy/EndPlay 清 Timer 与弱引用。D01 原点、D02 爆心和 D03 射向在接纳时保存；D03 `EmitterStartedAt` 在延迟结束时设定。

D03 从效果起动时间计算 `firstShotDelay + n * shotInterval`，只允许严格早于 `duration`；统一等待与既有首发延迟相加，默认首发距有效接纳命中时间见参数账本调参记录，工作期仍最多 12 发。迟到当前帧最多一发，错过时刻不补发、不续期；出生静态 Sweep/端点重叠仍使用配置后的 P01 球半径。方向提示跟随位置、绝对世界旋转与锁定三维射向一致。结束清发射/到期 Timer，已飞 P01 独立寿命，T13/T16 WorldSubsystem 管统一容量与整局清理。

角色保持 CharacterMovement，不给胶囊开启物理。`ReceiveImpulse` 对合法设备先 `WakeForExternalImpulse`，再缓存 EventId 去重事件；地面根不再用当前 IsSimulatingPhysics 作为唯一资格。帧末仍统一处理：当前速度加全部爆炸增量 → 柱子目标替换 Z → 统一限速；同帧多柱不叠加，回调次序不改变结果。角色只一次 LaunchCharacter，空中物理设备只一次 AddImpulse；Held/Spent/Active D01 种子不接收设备独立外力。

- **D01**：从激活位置沿世界 -Z 做 `JamGroundTrace`，只允许 WorldStatic，并校验关卡可承托标签与向上法线阈值，忽略 Pawn、道具和生成柱子。再检查**整个最终柱体体积**的静态空间，底部承托接触不算障碍；遇天花板、墙体或无合格表面都消耗种子并反馈失效。成功后种子关物理/碰撞/可见，保留逻辑 Active 到柱子结束，共用一个计数名额。柱子固定在落点，拆成视觉、上升查询、实体 Collider 三个组件：伸出时实体碰撞关闭，按上升路径检测 Pawn/JamDevice，`AffectedActors` 按 Actor 去重，每个对象弹一次。完全伸出后，在停留阶段检查完整实体体积，无角色/可移动道具占据才开启 WorldDynamic 的支撑/阻挡；有占据则等待净空，不强行开启挤压对象。停留结束前未净空则本次不启用实体。收回或被容量策略提前结束时先关实体碰撞、停弹射，再缓降网格。支撑尚未就绪时用较淡外观提示；启用后变实色，避免玩家误判可站立时机。
- **D02**：接纳时冻结世界爆心，统一延迟后按 Actor 去重收集角色与合法设备，根中心判半径，先快照代表点、静态遮挡及径向再执行可能重入的回调。角色伤害/合法推力使用本炸弹 ActivationHitId；Dormant 下游使用新 Guid、炸弹 SourceActor 与爆心到目标方向请求 TryActivate。Held/Active 不重启，死亡回调新增的掉落不自动加入本次查询快照；每次处理后复核世界/来源/目标状态。
- **D03**：接纳时以装置当前位置为原点，遍历当前世界有效存活 AEnemyBase，按 Actor 中心的三维距离选择 `EmitterTargetRangeCm` 内最近者（包含边界），锁定目标中心减装置位置的三维单位向量，不附加视线筛选。不持续追踪或在目标移动/死亡后重选；无目标采用 `Context.ShotDirection.GetSafeNormal2D()`，合法竖直方向回退固定世界 +X。定时器在统一延迟后使用当前装置位置与锁定方向生成 P01，机身旋转、地面固定、掉落、推力均不改射向。首发延迟、间隔和总工作时长来自参数账本；到期清 Timer、置 Spent。弹丸只避让生成者，出口静态受阻消耗该发，方向箭头读取同一世界向量。

## 5. Collision 基线

在 Project Settings 定义对象通道 `JamDevice/JamProjectile`，查询通道 `JamWeaponTrace/JamGroundTrace`。表中 B=Block、I=Ignore；对象碰撞两边都配置相同响应。角色只由 Capsule 接收战斗命中，Mesh 忽略，防止一次命中被处理两次。查询与对象响应是 UE 的不同配置项。[Epic：Collision Response Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/collision-response-reference-in-unreal-engine)

| 根组件对象 / 状态 | WorldStatic | WorldDynamic/柱体 | Pawn | JamDevice | JamProjectile | WeaponTrace | GroundTrace |
|---|---|---|---|---|---|---|---|
| 静态场景 WorldStatic | B | B | B | B | B | B | B |
| 角色 Capsule / Pawn | B | B | B | B | B | B | I |
| 空中设备 PhysicsBody 根 | B | B | I | B | I | I | I |
| 地面固定设备 PhysicsBody 根 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | I | I |
| Dormant / Active D03 ShotCollider，WorldDynamic / QueryOnly | I | I | I | I | B | B | I |
| P01 Sphere / JamProjectile，Query Only | B | B | B | B | I | I | I |
| 已启用实体柱体 / WorldDynamic | B | B | B | B | B | B | I |
| Held / Active D01 种子 / Spent | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | I | I |
| 柱子可见网格 / 未启用实体组件 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | I | I |

柱子查询不能仅依赖 SimulatePhysics 或设备实体根重叠：地面根无碰撞，需收集独立 ShotCollider 并按 Actor 去重/CanReceiveExternalImpulse 过滤，再显式唤醒。爆炸同样从查询球找到地面设备，以根中心和静态遮挡决定真实作用范围。D01 起柱仍只允许指定静态地面；机关和临时柱不影响导航，AI 使用静态 NavMesh。

## 6. 生命周期与验证

死亡先设置 `bDeathProcessed`，停 AI/射击，再请求一次掉落并注销存活敌人。掉落 Spawn/登记失败也不能再次掉落；容量淘汰顺序遵守 [道具与交互规则](../GDD/Device_Interaction_Rules.md)。Held/Active 仍计入场上机关数；Spent 进入时立即取消玩法 Timer、关闭碰撞并注销，短效果后 Destroy/EndPlay 再注销也无副作用。激活前先预占 Active 容量，同类满额先安全结束最老同类并释放名额；不能安全释放则拒绝本次激活、保持 Dormant 并提示，不能执行半个效果。D01 无地面/无生成空间属于已启动后的失效，需走 Spent 清理。

`AWaveDirector` 分开维护 `AliveCount` 与 `PendingSpawnCount`：排定批次时先登记全部待生成数，每个生成任务成功或明确取消时扣 Pending，成功才增加 Alive；敌人死亡或越界注销只扣一次 Alive。仅 **AliveCount==0 且 PendingSpawnCount==0** 才清波，不能在两批间无敌人时提前通关。生成间隔、安全距离、预警和重试读 [玩法参数基线](../GDD/Gameplay_Parameters.json) 的 flow/enemy；失败重试仍计 Pending。重开/结算显式取消未完成任务并归零，不能把暂时失败当作成功清波。

重开首版用重新加载主地图：先冻结生成/攻击，清 Timer、持有引用、弹丸和机关登记，再 OpenLevel；新局从关卡初值恢复。EndPlay 清理所有对象的 Timer/委托，回收弹丸先禁用碰撞并置 `bImpactProcessed`，再处理接口和 Destroy，阻止重复 Hit 或延迟回调。

编译器、默认地图、Cook 和首个独立包检查见 [UE 工程配置](../../04_Engine_Config/UE_Project_Configuration.md)，按 G0 执行；当前没有运行通过证据。

实现依次验收：空中投掷/方向锁定 → 连锁/幂等 → 角色与物理道具弹射 → 死亡掉落/容量 → 三波胜负/重开。每一步同时在 PIE 与独立包验证，具体案例见 [测试计划与验收](../../06_Test_Doc/Test_Plan.md)。日志记录 HitId、状态前后、方向、登记数和构建版本；开发调试线/日志受 Development 开关控制，提交包关闭。

新增源码与资产目录、命名和完整编译约定见 [代码与资产命名规范](../../03_Code_Standard/Coding_Conventions.md)。

提交范围、Git LFS、缓存忽略与恢复约定见 [版本管理规范](../../03_Code_Standard/Version_Control.md)。规则或参数变化按 [文档维护与变更流程](../../05_Development_Guide/Documentation_Workflow.md) 同步文档及回归测试。

## 7. T18 HUD、教学与菜单

`UMothHUDWidget` 在本地控制器拥有正式 Pawn/Rifle/Director 后读取保存的 WBP_MothHUD Designer 树，以 25 个必需 BindWidget 绑定数据控件与按钮；原生 WidgetTree 只作无 WBP 时的后备。布局、字体、普通颜色、容器层级由 Designer 管理；教学面板使用命名绑定，准星候选色结束后恢复 Designer 基色。菜单文案/页面预览在 Class Defaults，BlueprintPure GetHUDState 提供扩展读接口。初始 getter + AddUniqueDynamic 事件订阅，NativeTick 读取倒计时/换弹进度/准星/短提示；来源变更与 NativeDestruct 解绑，四按钮同样按构造/销毁绑定和解绑。E 的提示遵循 T07 首次准星阻挡命中与完整拾取条件，受伤/弹药/动作/波次显示来自生产对象。

2026-10-13 编辑接入：`DA_MothTutorial` 提供说明/反馈和世界布置；控制器从 HUD 选择的配置开始教学，Subsystem 复制为非 Standalone 的 Transient 本轮快照，结束清引用。预览只读取配置/示例数值，不绑定玩法来源或生成对象。默认位置、教学阈值及战斗参数沿用上一版；UI 具体制作由用户完成。Editor-only UMGEditor/UnrealEd/KismetCompiler 仅在 Editor 目标链接，初始化桥接只填空树，已有树保留并检查绑定；操作说明见 [UI_Setup](../../05_Development_Guide/UI_Setup.md)。

`UMothTutorialSubsystem` 持有世界内弱引用与一次 CheckTimer；激活/释放/靶死亡观察只记录来源与证据，计时器再转换步骤/清理自有对象。目标死亡必须来自该步 D02；D01 实际高度达标后独立保留；第二步必须曾真正投出且 W01 在空中启动，错误启动或落地自动补充；第三步 P01 的 SourceActor 保留源 D03，因此可区分 W01 直接启动。Ready 教学界外复用导演 Defeat 清理，世界结束清所有自有靶/标记/绑定。

菜单 UIOnly + 鼠标 + 主按钮焦点，教学/战斗 GameOnly + 视口焦点；切换取消持续按压，不把取消当作实际松键。Pause 不调用 SetGameplayEnabled(false)，所以合法换弹/恢复任务在世界暂停期间冻结、恢复继续。进入正式战斗 OpenLevel 重置所有状态；MothGameInstance 只有一位待开始意图，先绑定新玩家并成功 Preparing 后消费，结果重开清意图回到标题。原生激活观察回调后再次检查结束/状态/会话，防止回调结算后发出效果。
