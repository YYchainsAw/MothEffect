# Moth Effect（飞蛾效应）：道具基础重构接入

版本 v0.3 · 2026-10-17 · 状态：隔离 Editor 构建与 142 项 Automation 通过；硬件 PIE、调优与独立包待验收

2026-10-17 现行变更：三类道具 BeginPlay 后有出生保护期；D03 触发时自动锁定范围内最近存活敌人的三维射向，无目标才沿用入射 XY 投影。以下当前配置和验收预期已更新；2026-10-14 的 114 Editor / 12 Game 只代表当时规则，不能覆盖本轮新增行为。

当前 `refactor` 分支记录用户确认的四项道具改动：[#48 地面固定与尺寸](https://github.com/YYchainsAw/MothEffect/issues/48)、[#49 水平发射方向](https://github.com/YYchainsAw/MothEffect/issues/49)、[#50 统一激活延迟](https://github.com/YYchainsAw/MothEffect/issues/50)、[#51 爆炸激活](https://github.com/YYchainsAw/MothEffect/issues/51)。负责人 YYchainsAw、现有标签、M5 · 试玩与候选包验收与 Project / In Progress 均已回读；四项保持 Open，运行验收完成后再推进状态。

接口见 [DeviceBase.h](../../Source/MothEffect/Public/Devices/DeviceBase.h)，运动见 [DeviceMotion.cpp](../../Source/MothEffect/Private/Devices/DeviceMotion.cpp)，爆炸见 [DeviceBomb.cpp](../../Source/MothEffect/Private/Devices/DeviceBomb.cpp)。现行行为只在 [交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 维护，数值只在 [参数账本](../02_Design_Doc/GDD/Gameplay_Parameters.json) 维护。旧 Device/Emitter/Bomb/自然连锁接入中的历史验收保留，涉及本轮行为时以此文及现行契约为准。

## 1. 蓝图与实例配置

2026-10-16 补充：落地待拾取道具使用敌人大小胶囊，模型居中匀速旋转。Win64 Development Editor 构建通过，Devices、Art 与 EnemyDrops 共 50 项自动化全部通过，其中 4 项为新增掉落显示专项；实际图形 PIE 视觉与手感仍需验收。

现有 BP_DeviceLauncher、BP_DeviceBomb、BP_DeviceEmitter 继续使用 ADeviceBase，不新增道具类型或生命周期枚举。编译新源码后，在 Class Defaults 或场景实例 Details 搜索以下分类；已保存 BP/实例的覆盖值需逐项核对，不假定原生默认值覆盖已有资产。

| 编辑入口 | 属性 / 函数 | 含义与参数来源 |
|---|---|---|
| Device / Activation | SpawnActivationProtectionSeconds | BeginPlay 起的非负游戏时间保护期；来自 device.spawnActivationProtectionSeconds；期间弹道/爆炸激活拒绝，拾取/外力照常，放下/投掷不重置 |
| Device / Activation | ActivationDelaySeconds | 三类型共用非负延迟；默认来自 device.activationDelaySeconds。可为专项测试显式设零，生产默认保留统一等待 |
| Device / Activation | IsActivationPending | 只读等待标记；等待已属 Active，不是 Dormant，也没有新增 Pending 生命周期 |
| Device / Emitter | EmitterTargetRangeCm | 触发时最近存活 AEnemyBase 的三维搜索半径；来自 emitter.targetRangeCm；目标点为 Actor 中心，范围内无目标才使用入射水平后备 |
| Device / Size | VisualScale | 只调整显示倍数，范围来自 device.visualScaleMin/Max；不连带改变根或射击球 |
| Device / Size | BodyRadiusCm | 飞行碰撞、支持查询与安全释放体积；来自 device.bodyRadiusCm |
| Device / Size | ShotRadiusCm | 飞行与激活状态的命中扫描/P01 查询球；默认来自 throw.deviceShotColliderRadiusCm，实际半径不小于 Body |
| Device / Art | DroppedRotationDegreesPerSecond | 落地待拾取模型绕竖直轴匀速旋转的角速度，默认 60°/秒；0 表示停止旋转 |
| Device / Size | RefreshDeviceSize | 编辑器可调用，运行时修改尺寸后显式调用；更新球体与显示并重新检查承托 |
| Device / Motion | GroundSnapDistanceCm / GroundUpNormalMin | 支持短 Sweep 距离与法线阈值，来自 device 参数组；独立于 D01 起柱探地/标签规则 |
| Device / Motion | IsGrounded | 只读运动状态，独立于 Dormant/Held/Active/Spent |

调大小时分别决定视觉、飞行体积和射击判定，不把 Actor 总体缩放当作唯一入口。先刷新，再检查持物挂点、投掷释放空间、墙边净空、P01 枪口出生和射击遮挡。Shot 球较大可以帮助命中，但不能让墙后的设备绕过实际枪口/静态遮挡规则。

## 2. 地面固定与受力起飞

Dormant 与 Active D03 受到实际支持面承托时，停止物理、清速度、校正为直立显示，PhysicsBody 根使用 NoCollision，所以角色可穿行。落地 Dormant 使用与敌人一致的胶囊（半径 35 cm、半高 90 cm）作为 W01/P01 受击和准星拾取的 QueryOnly 体积，胶囊底部与物理根底部齐平；原 ShotCollider 查询球暂时关闭。模型的实际包围盒中心对齐胶囊中心，由独立 VisualPivot 匀速旋转，保留模型资源原有偏移、旋转和缩放。暂停会冻结旋转，VisualScale 只调整模型大小。拾取、投掷/受力起飞或激活后复原显示变换并关闭掉落胶囊，空中 Dormant 和 Active D03 使用原 ShotCollider。投掷以及合法机关外力通过真实空中物理运动，飞行根忽略 Pawn，仍受场景阻挡。

柱子与爆炸须从查询体积找到固定设备，用 CanReceiveExternalImpulse 过滤后经共用外力入口唤醒，再在帧末合并速度；不能仅用 IsSimulatingPhysics 或实体根重叠判断设备是否可作用。受力离地后再次承托会固定；原支持消失也会恢复下落。Held、Spent、Active D01 的已消耗种子不单独接受设备外力。EnemyDrops 接纳合法 grounded 出生，死亡幂等/容量/近邻搜索等原边界继续生效。生成/释放净空独立检查已有固定道具的实际根球体积，避免无碰撞道具实体重叠；较大的瞄准胶囊不扩大出生占用，邻近道具之间仍按准星首先命中的对象选择。

运动支持只要求实际阻挡设备根、合格向上法线且不是角色/道具/查询触发体，不要求 MothColumnSurface；D01 生成柱子仍另需 WorldStatic 与 MothColumnSurface 标签及最终静态净空。

## 3. 出生保护、接纳与自动瞄准

BeginPlay 记录世界游戏时刻，首段出生保护内 TryActivate 拒绝弹道和爆炸请求，不进入 Active、不占激活额度、不缓存迟到触发。保护期暂停冻结；拾取、放下、投掷和外部推力不重置起点。拾取和爆炸/柱子外力本身继续正常处理。出生保护与接纳后的效果延迟是两段独立规则。

TryActivate 成功时先预占容量、保存首次 HitId/方向/命中点并进入 Active，随后广播状态与激活证据。IsActivationPending 为 true 时不得拾取，重复命中不刷新方向/寿命/延迟。World Timer 到时复核状态、来源弱引用与世界许可，再执行一次核心效果；暂停冻结，FinishActivation/禁用/Destroy/EndPlay/重开取消旧等待。

D01 LauncherOrigin 和 D02 ExplosionCenter 在接纳时捕获；延迟期即使设备移动，本次原点/爆心仍使用该快照。D01 种子立即消耗并关物理/查询，柱子延迟后生成。D03 接纳时从装置当前位置搜索范围内最近有效存活 AEnemyBase，以三维直线距离比较 Actor 中心，包含范围边界，不要求额外视线通过。锁定指向该中心的三维单位向量；不随目标之后的移动/死亡重新选敌或修正射向。范围内无目标时才取入射 XY 投影，纯竖直合法命中回退世界 +X，完整零方向命中仍无效。方向提示、弹丸和出生点读取同一世界向量，滚动或玩家转向不改变射向。

统一激活延迟结束才开始 D03 工作寿命，随后仍等待 emitter.firstShotDelaySeconds 才首发；默认命中至首发时长为两段延迟之和，见参数 v0.19 调参记录。工作有效期不包含统一等待，严格早于到期的发射时刻才可生成；受阻、容量满额和迟帧均不补发或续期。

## 4. 爆炸连锁

D02 延迟结束时使用固定爆心，先按 Actor 去重并快照范围内代表点、静态遮挡与径向，再对角色结算伤害、向合法设备提交推力。无静态掩体遮挡的 Dormant 通过同一 TryActivate 接纳，随后等待该目标自身 ActivationDelaySeconds；Held 不激活，Active 不重启。范围以设备根中心判定，不以较大的 Shot 球边缘放大。

每个下游激活使用新 Guid，连锁炸弹以各自独立 ActivationHitId 结算本次伤害与推力；同一爆炸同一 Actor 只处理一次，后续爆炸不会被整个链共用的 ID 去重吞掉。方向从爆心指向目标；重合时采用本次激活方向水平投影，仍零则世界 +X。来源在回调中被结束/销毁时停止传播，死亡回调才新增的掉落不自动加入当前查询快照。

默认非零延迟把传播分成世界计时任务；零延迟配置也先提交 Active，已激活设备不可能沿回路再激活，因此传播受有限设备和幂等状态约束。暂停/结算/重开仍需取消全部世界任务和计数。

## 5. 原生验证与待执行验收

| 验证组 | 操作与可观察结果 | 当前状态 |
|---|---|---|
| 地面与飞行 | 三类型掉落/放下后固定，角色穿行；W01/P01 与 E 仍命中/拾取；投掷有真实飞行，柱子/爆炸可重新起飞、再落地固定，支持移除恢复下落 | 原生适用分支通过；硬件 PIE 待验收 |
| 尺寸与遮挡 | 分别修改 VisualScale、BodyRadiusCm、ShotRadiusCm 后刷新；安全释放和枪口出生按真实体积，Shot 不小于 Body，墙体继续阻挡射击/拾取 | 原生适用分支通过；硬件 PIE 待验收 |
| 出生保护 | 三类型在保护期内接受 W01/P01/D02 请求；暂停后继续计时，拾取/放下/投掷不重置，爆炸与柱子仍可施力 | 保护期拒绝激活且不占 Active 额度；结束后新请求成功；本轮自动化适用分支通过；硬件 PIE 待验收 |
| 自动射向与后备 | 近/远、高低、范围边界、死亡敌人和无敌人分别触发；随后移动目标/镜头/机身，核对 P01 和方向表现 | 最近存活目标按三维距离选定并锁定一次射向；无目标用 XY/竖直 +X；本轮自动化适用分支通过；硬件 PIE 待验收 |
| 延迟及取消 | 三类型效果在配置延迟后起动；重复命中不续等待；D03 验证两段首发时序；暂停、Finish、禁用、销毁、结算和重开不留下迟到效果 | 原生适用分支通过；硬件 PIE 待验收 |
| 爆炸链 | 布置无遮挡 D01/D02/D03 与掩体阴性对照；Held/Active 保护、下游延迟、多个炸弹对同一角色分别伤害但单次多组件去重；环状布局和显式零延迟不重复启动 | 原生适用分支通过；硬件 PIE 待验收 |
| 容量与流程 | 等待期计 Active，满额策略/闲置寿命/跨波/界外/重开仍一致；教学、方向反馈和主竞技场完整流程按新时序回归 | 原生适用分支通过；硬件 PIE 待验收 |

本地代码提交 `f6d6b2a`（Refs #48/#49/#50）和 `f91db81`（Refs #51/#48）只代表源码交付。收尾代码为 0e5370a / 4934627，实际隔离构建、114 Editor 与 12 主地图 Game 均通过，失败修正及告警保留在 [验证报告](../06_Test_Doc/Device_Refactor_Report_2026-10-14.md)。当前用户硬件 PIE、不同尺寸保存重载与手感、独立包分别继续验收；后续确认编辑器关闭，原工程 Editor 常规构建及六项 Foundation 复测通过，新 DLL 已就绪，重新打开项目即可加载。受影响测试为 TC03/04/05/06/07/08/10/11/12/13/14/15/16/18/19/20/23/31/32，完整测试计划由对应分工更新。


## 6. 2026-10-17 验证记录

本轮独立验证工程位于 `Saved/Automation/DeviceTargetingStage_20261017_01`，190 件 Source 文件与当前工作区逐字节 SHA256 一致，Content 只读引用原项目，没有保存资产。UE 5.8 Development Editor 完整构建通过；最终 Editor 自动化 142 项全部 Success（139 无警告、3 项带警告），无失败或未执行项。新增的出生精确边界、真实爆炸保护、最近存活敌人三维锁向、范围外后备与范围边界 5 项全部通过。警告来自 Paragon 旧 Rig/输入映射和 DesignerProbe 临时绑定检查。

证据：[完整构建](../06_Test_Doc/Evidence/DeviceTargeting/Build_Full_2026-10-17.log)、[最终增量构建](../06_Test_Doc/Evidence/DeviceTargeting/Build_Final_2026-10-17.log)、[逐项 Automation 报告](../06_Test_Doc/Evidence/DeviceTargeting/Automation_2026-10-17.json)、[源码 SHA256](../06_Test_Doc/Evidence/DeviceTargeting/SourceManifest_2026-10-17.json)。新增 Emitter 测试手动推进隔离世界时钟，方向断言覆盖真实生成的 P01，不代表真实渲染/完整整局命中率验收；既有链式运动分支执行真实 World Tick。TC43–TC45 完整硬件 PIE 仍待执行。

原工程编辑器在验证期间保持打开，原 DLL 未替换；本轮新增反射字段，须保存并关闭该编辑器后完整编译 `MothEffectEditor / Development Editor / Win64`，再重开项目加载新规则。
