# motheffect：技术设计

版本 v0.5 · 2026-10-03 · 游戏名：motheffect

用户已确认 UE 5.8、蓝图与 C++ 混合开发、正式显示名 motheffect、魔法朋克风格。当前工程文件为 [MothEffect.uproject](../../../MothEffect.uproject)，已有第三人称模板源码与资源。本次只读核对文件，未修改或运行游戏实现；自定义玩法、编译与打包结果尚未验收。玩法以 [道具与交互规则](../GDD/Device_Interaction_Rules.md) 为准，数值只维护在 [玩法参数基线](../GDD/Gameplay_Parameters.json)。

## 1. 起步与职责

建议沿用现有 Windows C++ Third Person 工程，以模板角色、移动动画与输入作为起点。空白工程适用于需要从零定制角色框架或已有基础资源的情况；本作单人周期内，模板能省去重新搭建这些基础功能的工作。用基础几何体表现枪、机关和敌人，先验证空中触发，再补持枪/投掷表现；模板本身不代表本作越肩射击系统已经接入。[Epic：Third Person 模板](https://dev.epicgames.com/documentation/unreal-engine/third-person-template-in-unreal-engine)

已存在的模板源码、资产、模块依赖与配置核对记录见 [UE 工程配置](../../04_Engine_Config/UE_Project_Configuration.md)；类与蓝图映射沿用下表。


C++ 管状态、命中、伤害、推力、定时器和容量；蓝图管组件组装、相机参数、魔法朋克材质、音效、特效、界面和关卡引用。蓝图调用 C++ 入口，不另写一套伤害或激活流程。继续使用已有运行时模块 `MothEffect`，不新建 `Jam` 模块；第一版不引入 GAS、Mass、自定义插件或联网框架。

| C++ 类 / 父类 | 蓝图资产 | 责任 |
|---|---|---|
| `AMothEffectCharacter : ACharacter` | `BP_ThirdPersonCharacter` | 移动、越肩相机、拾取/投掷、输入门控、玩家受推力 |
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
| `UDeviceDefinition : UDataAsset` | `DA_Device_D01 / D02 / D03` | `FDeviceConfig` 与对应 BP 类/资源引用 |

## 2. 数据与调用契约

以下名称、签名均为**本项目规划的自定义 API**，不是 UE 内建功能，也不是已经可编译的代码。实现时再补反射宏、导出宏和头文件。

- `EDeviceState`：`Dormant, Held, Active, Spent, Destroyed`；`Destroyed` 是清理末态，可不留存于已销毁对象。
- `EDeviceKind`：`Launcher, Bomb, Emitter`。
- `FHitContext`（蓝图显示名 `ST_HitContext`）：`FGuid HitId`、`AActor* SourceActor`、`APawn* InstigatorPawn`、`FVector ImpactPoint`、`FVector ShotDirection`、`float Damage`。方向为归一化世界飞行方向，不用表面法线；引用仅在命中处理期间使用并验证有效性。
- `FDeviceConfig`（蓝图显示名 `ST_DeviceConfig`）：包含 `EDeviceKind Kind` 及对应 launcher/bomb/emitter 参数；字段与 [玩法参数基线](../GDD/Gameplay_Parameters.json) 对应，单位为 cm、秒、cm/s。伤害与容量分别来自 weapon/projectile/limits 组。

| 自定义入口 / 事件签名 | 调用约定 |
|---|---|
| `UBallisticReactive / IBallisticReactive`：`bool ReceiveBallisticHit(const FHitContext& Context)` | 角色转交 Health，机关转交 TryActivate；返回 true 只代表规则被接受 |
| `UImpulseReceiver / IImpulseReceiver`：`void ReceiveImpulse(FVector Velocity, bool bOverrideZ, FGuid EventId)` | 当帧缓存，EventId 去重；帧末先合并爆炸增量、再用柱子目标替换 Z、最后统一限速 |
| `ADeviceBase::bool TryActivate(const FHitContext& Context)` | 仅 Dormant 且容量许可可成功；需释放旧同类时先按 [道具与交互规则](../GDD/Device_Interaction_Rules.md) 结束，获准后置 Active，再执行效果 |
| `ADeviceBase::bool TryPickup(AMothEffectCharacter* Picker)` | 距离、遮挡、状态均满足才置 Held |
| `AMothEffectCharacter::bool TryReleaseHeldDevice(bool bThrow)` | 统一放下/投掷安全检查；失败保持 Held |
| `UHealthComponent::bool ApplyHit(const FHitContext& Context)` | HitId 去重；死者不再扣血；返回是否接受伤害 |
| `OnDeviceStateChanged(ADeviceBase* Device, EDeviceState OldState, EDeviceState NewState)` | C++ 修改状态后广播，蓝图只更新表现 |
| `OnHealthChanged(float CurrentHealth, float MaxHealth)`；`OnDied(AActor* Victim, AActor* SourceActor)` | 动态多播委托；每个角色 OnDied 最多一次 |
| `AWaveDirector::bool TryRegisterDevice(ADeviceBase* Device)`；`void UnregisterDevice(ADeviceBase* Device)` | 登记/注销均幂等，不能在 UI 中另行计数 |

**阻挡命中即消耗 P01，无论接口返回 true 或 false。** 墙体、Active D03 和已经启用实体碰撞的柱子阻挡弹丸；Held 本身关闭碰撞，弹丸可继续命中玩家。接口拒绝激活不返还弹丸，也不返还步枪弹药。爆炸和普通物理碰撞不调用 Ballistic 接口。

[玩法参数基线](../GDD/Gameplay_Parameters.json) 是设计数值权威，运行时从 DataAsset/默认配置读取；本版不承诺自动导入 JSON。首次录入与每次调参都核对 [玩法参数基线](../GDD/Gameplay_Parameters.json)，禁止在 Tick、关卡脚本或特效里藏伤害数值。

## 3. 越肩射击与输入

W01：①从相机沿准星射线找第一阻挡点；无命中则取射程终点。②从枪口向该点再检测，只有第二段实际命中才造成伤害/启动机关，`ShotDirection` 使用第二段方向。两段忽略玩家、枪及 Held 道具。先检测角色胸前到枪口的短段，枪口已被墙体阻挡或嵌入时取消射击伤害，提示“枪口受阻”，避免镜头看到目标却从墙另一侧开枪。SpringArm 防碰撞负责镜头，不能代替枪口检测。[Epic：单次射线返回首个阻挡命中](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Collision/LineTraceByChannel)

复用已有 /Game/Input 的 IMC_Default、IMC_MouseLook 与 IA_Move/Look/MouseLook/Jump，在现有控制器与角色输入绑定上扩展。新增 IMC_MothGameplay 只映射 IA_Primary/Interact/Aim/Reload/Sprint，不重复建立第二套移动与跳跃输入；新增输入资产放 /Game/MothEffect/Player/Input。`IA_Primary` 用简单 Boolean，不叠加 Hold/Release 触发器：Started 时若 Held，尝试投掷，并锁定本次按键为投掷；否则开枪。持续 Triggered 仅在本次为射击且满足间隔时开火。Completed 或 Canceled 停止连射并解除该次锁定；投掷后还需满足 `throw.minGunRecoverySeconds`，松键和恢复计时两项均完成才允许下一次 Started 开枪，失败投掷也须重新按键。暂停、失焦、死亡清除射击定时器，恢复游戏时若鼠标仍按住，先等待松开。这样一记投掷不会同帧把道具射爆。[Epic：Enhanced Input 事件](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)

E 在空手时拾取准星附近的最近合格 Dormant，道具到角色需无遮挡；Held 时安全放下，Held 时 R 换弹不生效。拾取关物理、关碰撞并附着手部挂点；投掷先从角色外侧到释放点做与道具体积一致的 Sweep，再检查终点重叠，不能把释放点直接放到摄像机位置。路径阻挡或终点不合法则保持 Held 并提示。Detach 后切回 Dormant、开启重力与物理，设置 [玩法参数基线](../GDD/Gameplay_Parameters.json) 的初速度；可对物理根启用 CCD 并验证薄墙。放下同样检查空位，反复拾取放下不刷新累计闲置寿命。

## 4. 物理与三种机关

机关用一个 USphereComponent 作物理和射击判定根，视觉挂件不另模拟物理或接收命中。Dormant/投掷/Active 发射器由 Chaos 物理移动，重力始终开启；不同时用 ProjectileMovement 移动机关。P01 则用 Sphere 根 + ProjectileMovement，不启用根的 Simulate Physics，以 Sweep 检测命中，按 [玩法参数基线](../GDD/Gameplay_Parameters.json) 设置 gravityScale=0，保持直线飞行。[Epic：ProjectileMovement 与物理模拟关系](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UProjectileMovementComponent)

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
