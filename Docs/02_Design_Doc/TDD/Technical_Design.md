# Moth Effect（飞蛾效应）：技术设计

版本 v0.18 · 2026-10-08 · 游戏名：Moth Effect（飞蛾效应）

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
| `AEnemyBase : ACharacter` | `BP_EnemyBase` | 射击 AI、死亡、固定类型掉落；用内建 AIController |
| `AWaveDirector : AActor` | `BP_WaveDirector` | 波次、生成点、存活敌人和机关容量登记 |
| `UHealthComponent : UActorComponent` | 角色上的组件 | HP、一次死亡事件 |
| `UAnimInstance`（UE 内建父类，先不新增 C++ 动画类） | `ABP_MothPlayer`（用户已创建） | 读取玩家与移动组件快照，执行移动状态机、AimOffset、上半身分层；不修改玩法状态 |
| `UDeviceDefinition : UDataAsset` | `DA_Device_D01 / D02 / D03` | `FDeviceConfig` 与对应 BP 类/资源引用 |

拾取、放下、投掷先保留在 Character 的独立方法中；复杂度增加后再抽成交互组件，并同步本表。枪械只由 ARifle 维护弹匣、射速和换弹任务，HealthComponent 只维护一份血量，Character 只维护一份 Held 引用，不额外创建重复持有这些数据的 WeaponComponent 或背包系统。

## 2. 数据与调用契约

以下名称、签名为**本项目自定义 API 契约**，不是 UE 内建功能。玩家行动/左键枚举、FHitContext、BallisticReactive、ARifle、HealthComponent 及相关事件已写入原生源码，T05 当前 PIE 分支由用户报告通过。T06 的 EDeviceState/EDeviceKind、ADeviceBase::TryActivate 与 OnDeviceStateChanged 已实现，用户当前状态壳测试通过；T07 的 TryPickup、TryReleaseHeldDevice 已实现，Automation 与基础 PIE 通过，适用边界待验收。机关效果、容量、推力与波次等后续入口仍为计划接口。

- `EDeviceState`：`Dormant, Held, Active, Spent, Destroyed`；`Destroyed` 是清理末态，可不留存于已销毁对象。
- `EDeviceKind`：`Launcher, Bomb, Emitter`。
- `EPlayerActionState`：`Ready, Carrying, ThrowRecovery, Reloading, Dead`；由 Character 唯一持有。`Ready` 表示持枪可接收动作请求，实际开火还需弹药、射速、按压与局内许可检查。
- `EPrimaryPressMode`：`None, Fire, Throw, Blocked`；记录本次左键按压归属。它是输入会话状态，不替代玩家行动状态。
- `FHitContext`（蓝图显示名 `ST_HitContext`）：`FGuid HitId`、`AActor* SourceActor`、`APawn* InstigatorPawn`、`FVector ImpactPoint`、`FVector ShotDirection`、`float Damage`。方向为归一化世界飞行方向，不用表面法线；引用仅在命中处理期间使用并验证有效性。
- `FDeviceConfig`（蓝图显示名 `ST_DeviceConfig`）：包含 `EDeviceKind Kind` 及对应 launcher/bomb/emitter 参数；字段与 [玩法参数基线](../GDD/Gameplay_Parameters.json) 对应，单位为 cm、秒、cm/s。伤害与容量分别来自 weapon/projectile/limits 组。

| 自定义入口 / 事件签名 | 调用约定 |
|---|---|
| `UBallisticReactive / IBallisticReactive`：`bool ReceiveBallisticHit(const FHitContext& Context)` | 角色转交 Health，机关转交 TryActivate；返回 true 只代表规则被接受 |
| `UImpulseReceiver / IImpulseReceiver`：`void ReceiveImpulse(FVector Velocity, bool bOverrideZ, FGuid EventId)` | 当帧缓存，EventId 去重；帧末先合并爆炸增量、再用柱子目标替换 Z、最后统一限速 |
| `ADeviceBase::bool TryActivate(const FHitContext& Context)` | 仅 Dormant 且容量许可可成功；需释放旧同类时先按 [道具与交互规则](../GDD/Device_Interaction_Rules.md) 结束，获准后置 Active，再执行效果 |
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

**阻挡命中即消耗 P01，无论接口返回 true 或 false。** 墙体、Active D03 和已经启用实体碰撞的柱子阻挡弹丸；Held 本身关闭碰撞，弹丸可继续命中玩家。接口拒绝激活不返还弹丸，也不返还步枪弹药。爆炸和普通物理碰撞不调用 Ballistic 接口。

[玩法参数基线](../GDD/Gameplay_Parameters.json) 是设计数值权威，运行时从 DataAsset/默认配置读取；本版不承诺自动导入 JSON。首次录入与每次调参都核对 [玩法参数基线](../GDD/Gameplay_Parameters.json)，禁止在 Tick、关卡脚本或特效里藏伤害数值。

## 3. 越肩射击与输入

W01：①从相机沿准星射线找第一阻挡点；无命中则取射程终点。②从枪口向该点再检测，只有第二段实际命中才造成伤害/启动机关，`ShotDirection` 使用第二段方向。两段忽略玩家、枪及 Held 道具。先检测角色胸前到枪口的短段，枪口已被墙体阻挡或嵌入时取消射击伤害，提示“枪口受阻”，避免镜头看到目标却从墙另一侧开枪。SpringArm 防碰撞负责镜头，不能代替枪口检测。[Epic：单次射线返回首个阻挡命中](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Collision/LineTraceByChannel)

复用已有 /Game/Input 的 IMC_Default、IMC_MouseLook 与 IA_Move/Look/MouseLook/Jump，在现有控制器与角色输入绑定上扩展。新增 IMC_MothGameplay 只映射 IA_Primary/Interact/Aim/Reload/Sprint，不重复建立第二套移动与跳跃输入；新增输入资产沿用户实际目录，Action 放 /Game/MothEffect/Input/Actions，Mapping Context 放 /Game/MothEffect/Input/Mapping。`IA_Primary` 用简单 Boolean，不叠加 Hold/Release 触发器；单次交互与换弹绑定 Started，移动/视角轴使用 Triggered。左键一次按压流程与取消语义见第 3.3 节。[Epic：Enhanced Input 事件](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)

E 在空手时拾取准星附近的最近合格 Dormant，道具到角色需无遮挡；Held 时安全放下，Held 时 R 换弹不生效。拾取关物理、关碰撞并附着手部挂点；投掷先从角色外侧到释放点做与道具体积一致的 Sweep，再检查终点重叠，不能把释放点直接放到摄像机位置。路径阻挡或终点不合法则保持 Held 并提示。Detach 后切回 Dormant、开启重力与物理，设置 [玩法参数基线](../GDD/Gameplay_Parameters.json) 的初速度；可对物理根启用 CCD 并验证薄墙。放下同样检查空位，反复拾取放下不刷新累计闲置寿命。

### 3.1 玩家状态与转换

行动、移动和瞄准分别表达。移动由 CharacterMovement 的 MovementMode/IsFalling 与速度读取；瞄准记录输入意图和实际是否生效；开火是在 Ready 中按射速执行的动作，不创建独占 Firing 状态。不把空中、瞄准、携带的所有组合展开成一个大枚举，也不为玩家另建 StateTree。

| 行动状态 | 进入条件与权威数据 | 可执行行为 | 退出与失败处理 |
|---|---|---|---|
| Ready | 存活、无持物、无换弹或投掷恢复任务 | 移动/跳跃、瞄准、冲刺；左键申请射击，E 拾取，R 手动换弹 | 拾取成功到 Carrying；有效换弹到 Reloading；请求失败保持 Ready |
| Carrying | 有一个有效 Held 引用，所属机关也为 Held | 移动/跳跃、瞄准投掷方向、非瞄准冲刺；E 放下，左键单次投掷 | 安全放下到 Ready；投掷成功到 ThrowRecovery；阻挡保持 Carrying；不响应 R 或枪械射击 |
| ThrowRecovery | 物理释放已提交，Held 引用已清空，恢复计时进行中 | 移动/跳跃、瞄准、非瞄准冲刺；不接受新的射击/拾取/换弹请求 | 恢复计时到期到 Ready；提前左键为 Blocked，不缓存开火；放下不经过本状态 |
| Reloading | ARifle 已接受有效换弹请求 | 移动/跳跃、瞄准、非瞄准冲刺；E 尝试拾取；不接受射击或重复换弹 | 计时完成到 Ready 并补弹；拾取成功取消任务到 Carrying；拾取失败继续；普通受伤不取消 |
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

速度从 `player.walkSpeedCmPerSec / aimMoveSpeedCmPerSec / sprintSpeedCmPerSec` 读取；瞄准优先于冲刺，瞄准与有效开火会取消当前冲刺，需松开再按 Shift 才重新请求冲刺，避免松开瞄准就意外恢复跑步。携带不禁用跳跃或非瞄准冲刺；瞄准移动速度首个原型沿用走路速度，按 TC34 实测后再调整。

空中控制使用 `player.airControl`，起步复用本地源码默认值；受柱子/爆炸推动后不进入眩晕状态，仍能转镜头、瞄准和在行动许可下射击。跳跃只在 CharacterMovement 允许时接受，不增加二段跳；落地不强制锁输入，首版无坠落伤害。相机臂长、越肩偏移、FOV/混合时间读取 camera 组，投掷与射击共用准星视线目标；相机碰撞不代替释放体积或枪口检测。

### 3.3 左键、换弹与中断

1. Started 先检查全局许可和 `bRequirePrimaryRelease`。Carrying 将本次归属设为 Throw，尝试一次安全投掷；失败也保持 Throw 到实际松键。Ready 且允许射击时设为 Fire，由 ARifle 执行首发并用单一 Timer 连射；其他情况设为 Blocked。Primary 的 Triggered 不再独立开枪，避免同次按下重复首发。
2. ARifle 每发检查玩家仍为 Ready、本次归属仍为 Fire、局内许可、射速和弹药。Completed 停止连射并在确认物理松键后清本次归属；Canceled 停连射、置 Blocked 并设置 `bRequirePrimaryRelease`，实际松键后才清门控，不能把输入上下文移除造成的取消当作玩家已松键。
3. 成功投掷清 Held、恢复持枪表现、进入 ThrowRecovery，计时长度只读取 `throw.minGunRecoverySeconds`。同次长按始终不能开枪；松键与恢复计时两个条件顺序不限，但都完成后再出现新的 Started 才能射击。恢复期间再次按下只显示恢复提示，不在计时到期时自动开枪，必须再松键重新按；失败投掷不启动恢复计时。
4. 换弹时先停止连射；空匣不开枪且不自动换弹，R 才请求换弹，满匣拒绝无效请求。换弹计时只由 ARifle 持有；完成一次补弹，取消不补弹。拾取候选完整验证成功后统一取消未完成换弹并提交 Carrying，不能先取消再发现拾取失败。取消/死亡使任务标识失效，迟到回调不补弹或覆盖新行动。
5. 暂停冻结玩法与动画计时，保留合法换弹/恢复剩余时间，同时停连射、清瞄准/冲刺输入意图并设置松键门控。失焦也停连射并设置门控；Canceled 或输入 Flush 本身不能解除门控，重新获得焦点后检查实际按键已松开，之后才允许新 Started。死亡/胜负/EndPlay 取消玩法任务、停止攻击并解除无效引用/绑定；结算后的回调不得继续伤害或补弹。

### 3.4 动画资产与图结构

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
| Grounded | 局部水平速度的二维 BlendSpace；现有资产以等价的 Direction / GroundSpeed 表达，复用八方向 Walk/Jog 和待机 | 侧移/倒退姿态与真实移动方向一致 |
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

机关使用 USphereComponent 物理根 PhysicsBody，另挂 QueryOnly 的 ShotCollider 球用于射击判定；视觉挂件不另模拟物理或接收命中。Held 关闭两球碰撞与物理；安全释放按物理根实际半径及响应检查空间，不使用较大的射击球替代。Dormant/投掷/Active 发射器由 Chaos 物理移动，重力始终开启；不同时用 ProjectileMovement 移动机关。P01 则用 Sphere 根 + ProjectileMovement，不启用根的 Simulate Physics，以 Sweep 检测命中，按 [玩法参数基线](../GDD/Gameplay_Parameters.json) 设置 gravityScale=0，保持直线飞行。[Epic：ProjectileMovement 与物理模拟关系](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UProjectileMovementComponent)

T08 当前碰撞接入：`Config/DefaultEngine.ini` 新增 `JamProjectile` 对象通道（`ECC_GameTraceChannel1`，默认 Block），映射统一维护在 `MothCollisionChannels.h`。P01 QueryOnly 球忽略同类弹丸、Visibility、Camera，并通过 MoveIgnoreActors 忽略自己的生成者；装置物理根忽略 P01，由较大的 ShotCollider 阻挡 P01。Held/Spent/Destroyed 仍关闭两球。步枪继续使用既有 Visibility；完整 JamDevice/JamWeaponTrace/JamGroundTrace 矩阵后续任务逐步接入，不能只改一端通道。蓝图若覆盖响应，接入时应按当前矩阵核对。

T08 发射调度：从激活时间计算 `firstShotDelay + n * shotInterval`，只允许严格早于 `duration` 的时刻；默认无遮挡且正常帧率最多 12 发。迟到回调在当前帧最多生成一发，跳过错过的时刻，到期后不补发、不延长寿命。出生前用配置后的 P01 球半径及组件缩放检查物理根中心到枪口的静态 Sweep 与端点静态重叠，阻挡则消耗该发；其余目标的出生短段阻挡交给同一个 P01 命中入口。方向提示与机身共享位置但使用绝对旋转；`FinishActivation`、禁用玩法、Destroy/EndPlay 清理发射和到期 Timer。已飞出的 P01 继续自己的寿命；容量和本局统一结算清理仍待 T13/T16，当前不得据此宣称全局限制已实现。

角色保持 CharacterMovement，不给胶囊开启物理。`ReceiveImpulse` 在角色/机关实现内缓存事件，按逻辑帧统一结算：当前速度加本帧全部爆炸增量 → 如有柱子事件，将 Z 替换为角色/道具的 launcher 对应目标速度 → 对角色和机关都按 [玩法参数基线](../GDD/Gameplay_Parameters.json) 统一上限 Clamp。同帧多柱目标相同，不重复叠加；回调先后不影响结果。结算得到最终速度后，角色只调用一次 `LaunchCharacter(FinalVelocity, true, true)`；物理机关只对根调用一次 `AddImpulse(FinalVelocity-CurrentVelocity, NAME_None, true)`。角色 Launch 在下一移动 Tick 生效，不能在每个命中回调里分别 Launch 覆盖待处理速度。没有柱子时保留爆炸叠加结果；没有爆炸时柱子保留原 XY 并替换 Z。[Epic：LaunchCharacter](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ACharacter/LaunchCharacter)；[Epic：Add Impulse](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Physics/AddImpulse)

- **D01**：从激活位置沿世界 -Z 做 `JamGroundTrace`，只允许 WorldStatic，并校验关卡可承托标签与向上法线阈值，忽略 Pawn、道具和生成柱子。再检查**整个最终柱体体积**的静态空间，底部承托接触不算障碍；遇天花板、墙体或无合格表面都消耗种子并反馈失效。成功后种子关物理/碰撞/可见，保留逻辑 Active 到柱子结束，共用一个计数名额。柱子固定在落点，拆成视觉、上升查询、实体 Collider 三个组件：伸出时实体碰撞关闭，按上升路径检测 Pawn/JamDevice，`AffectedActors` 按 Actor 去重，每个对象弹一次。完全伸出后，在停留阶段检查完整实体体积，无角色/可移动道具占据才开启 WorldDynamic 的支撑/阻挡；有占据则等待净空，不强行开启挤压对象。停留结束前未净空则本次不启用实体。收回或被容量策略提前结束时先关实体碰撞、停弹射，再缓降网格。支撑尚未就绪时用较淡外观提示；启用后变实色，避免玩家误判可站立时机。
- **D02**：激活时冻结世界爆心，用一次球形查询收集角色/可移动机关，按 Actor 去重；遮挡规则遵守 [道具与交互规则](../GDD/Device_Interaction_Rules.md)。伤害只给 Health，推力只给 ImpulseReceiver，两条路径不能顺便触发机关。
- **D03**：激活保存 `LockedDirection = Context.ShotDirection`，定时器使用当前装置位置和固定世界方向生成 P01；机身旋转、掉落、推力都不改方向。首发延迟、间隔和总时长来自 [玩法参数基线](../GDD/Gameplay_Parameters.json)；到期清 Timer、置 Spent。弹丸忽略生成它的 Actor，除此不按阵营过滤；出生点到出口先检测，出口被墙挡住就消耗该发，不在墙后生成。方向箭头按锁定方向绘制。

## 5. Collision 基线

在 Project Settings 定义对象通道 `JamDevice/JamProjectile`，查询通道 `JamWeaponTrace/JamGroundTrace`。表中 B=Block、I=Ignore；对象碰撞两边都配置相同响应。角色只由 Capsule 接收战斗命中，Mesh 忽略，防止一次命中被处理两次。查询与对象响应是 UE 的不同配置项。[Epic：Collision Response Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/collision-response-reference-in-unreal-engine)

| 根组件对象 / 状态 | WorldStatic | WorldDynamic/柱体 | Pawn | JamDevice | JamProjectile | WeaponTrace | GroundTrace |
|---|---|---|---|---|---|---|---|
| 静态场景 WorldStatic | B | B | B | B | B | B | B |
| 角色 Capsule / Pawn | B | B | B | B | B | B | I |
| Dormant / Active D03，JamDevice | B | B | B | B | B | B | I |
| P01 Sphere / JamProjectile，Query Only | B | B | B | B | I | I | I |
| 已启用实体柱体 / WorldDynamic | B | B | B | B | B | B | I |
| Held / Active D01 种子 / Spent | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | I | I |
| 柱子可见网格 / 未启用实体组件 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | 无碰撞 | I | I |

柱子用独立查询体积取得 Pawn/JamDevice，不依靠实体 Collider 的 Hit 触发弹射。只允许指定静态地面承托；静态墙虽响应 GroundTrace，命中仍需表面校验。机关和临时柱子不影响导航，AI 用灰盒静态 NavMesh；AI 路径可能被柱子暂时阻挡，需做停滞超时后换位置的原型验证，先不启用动态导航重建。

## 6. 生命周期与验证

死亡先设置 `bDeathProcessed`，停 AI/射击，再请求一次掉落并注销存活敌人。掉落 Spawn/登记失败也不能再次掉落；容量淘汰顺序遵守 [道具与交互规则](../GDD/Device_Interaction_Rules.md)。Held/Active 仍计入场上机关数；Spent 进入时立即取消玩法 Timer、关闭碰撞并注销，短效果后 Destroy/EndPlay 再注销也无副作用。激活前先预占 Active 容量，同类满额先安全结束最老同类并释放名额；不能安全释放则拒绝本次激活、保持 Dormant 并提示，不能执行半个效果。D01 无地面/无生成空间属于已启动后的失效，需走 Spent 清理。

`AWaveDirector` 分开维护 `AliveCount` 与 `PendingSpawnCount`：排定批次时先登记全部待生成数，每个生成任务成功或明确取消时扣 Pending，成功才增加 Alive；敌人死亡或越界注销只扣一次 Alive。仅 **AliveCount==0 且 PendingSpawnCount==0** 才清波，不能在两批间无敌人时提前通关。生成间隔、安全距离、预警和重试读 [玩法参数基线](../GDD/Gameplay_Parameters.json) 的 flow/enemy；失败重试仍计 Pending。重开/结算显式取消未完成任务并归零，不能把暂时失败当作成功清波。

重开首版用重新加载主地图：先冻结生成/攻击，清 Timer、持有引用、弹丸和机关登记，再 OpenLevel；新局从关卡初值恢复。EndPlay 清理所有对象的 Timer/委托，回收弹丸先禁用碰撞并置 `bImpactProcessed`，再处理接口和 Destroy，阻止重复 Hit 或延迟回调。

编译器、默认地图、Cook 和首个独立包检查见 [UE 工程配置](../../04_Engine_Config/UE_Project_Configuration.md)，按 G0 执行；当前没有运行通过证据。

实现依次验收：空中投掷/方向锁定 → 连锁/幂等 → 角色与物理道具弹射 → 死亡掉落/容量 → 三波胜负/重开。每一步同时在 PIE 与独立包验证，具体案例见 [测试计划与验收](../../06_Test_Doc/Test_Plan.md)。日志记录 HitId、状态前后、方向、登记数和构建版本；开发调试线/日志受 Development 开关控制，提交包关闭。

新增源码与资产目录、命名和完整编译约定见 [代码与资产命名规范](../../03_Code_Standard/Coding_Conventions.md)。

提交范围、Git LFS、缓存忽略与恢复约定见 [版本管理规范](../../03_Code_Standard/Version_Control.md)。规则或参数变化按 [文档维护与变更流程](../../05_Development_Guide/Documentation_Workflow.md) 同步文档及回归测试。
