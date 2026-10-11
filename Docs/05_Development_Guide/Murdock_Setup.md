# Moth Effect（飞蛾效应）：Murdock 与一体枪械接入

版本 v0.1 · 2026-10-08 · C++ 与回归测试源码已写入；未编译、未运行 UE，资产由用户配置

用户要求使用 `/Game/ParagonMurdock` 的人物和配套动画，枪械与人物属于同一个 Skeletal Mesh；取消走路/冲刺两个速度分支，移动统一为 Jog。助手继续负责 C++ 和接入说明，用户负责 UE 编译、资产配置与 PIE。旧模板的局部验收不能作为这次 Murdock 迁移已经通过的证据。

## 1. 先加载新代码

保存编辑器工作、关闭 UE，用户自行编译后重新打开工程。这次移除了 `WalkSpeed / AimMoveSpeed / SprintSpeed`，新增 `JogSpeed`，不能只修改旧会话的 CharacterMovement.MaxWalkSpeed 代替代码加载。

`JogSpeed` 是正常、瞄准、携带时的唯一速度来源，默认值与 [参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 的 `player.jogSpeedCmPerSec` 一致。C++ 不再绑定 IA_Sprint；旧 DoSprintStart/End 和 CancelSprintUntilRelease 保留为兼容入口，不改变移动方式，IsSprinting 始终为 false。移动、跳跃、相机、瞄准 FOV、开火、换弹和交互继续使用现有接口。

## 2. 配置现有 Murdock 玩家

静态核对到用户已创建 `/Game/MothEffect/Characters/Player/BP_MothPlayer`、`/Game/MothEffect/Animations/Player/ABP_MothMurdock`、`/Game/MothEffect/Framework/BP_MothGameMode` 与 BP_MothPlayerController；当前 DefaultEngine 已指向 BP_MothGameMode。文件引用存在不替代编辑器或运行验证。

打开 BP_MothPlayer，确认父类为 MothEffectCharacter，Class Defaults 按以下配置：

| 字段 | 设置 |
|---|---|
| Player → Movement → Jog Speed | 使用参数基线的 Jog 速度；新字段原生默认值已同步 |
| Player → Weapon → Use Integrated Weapon Mesh | 勾选 |
| Player → Weapon → Integrated Muzzle Socket | `Muzzle`，或实际枪管前端的 Socket 名 |
| Player → Weapon → Rifle Class | 保留 BP_Rifle 的射速/弹匣/调试设置；留空时，一体模式使用原生 Rifle |
| Input → Primary Action | IA_Primary |
| Input → Reload Action | IA_Reload |
| Input → Aim Action | IA_Aim |
| Input → Interact Action | IA_Interact |

一体模式中，RifleComponent 是隐藏的玩法 Actor，负责射速、弹药、换弹、射线伤害和事件；不渲染第二把枪、不要求旧 WeaponSocket。每发射击从角色 Mesh 的 Integrated Muzzle Socket 读取世界位置，继续执行相机目标、身体到枪口检查和枪口重叠检查。枪口未配置或来源销毁时拒绝射击，不回退到角色中心/旧枪位置。

本地 Murdock_Skeleton 资产含 `Muzzle` 名称；请在 Skeletal Mesh/Skeleton 编辑器检查它是否位于实际枪管前端，并检查动画播放时是否随枪移动。选择 Muzzle_01 等其他挂点时，以预览结果为准，不能只凭名称猜测用途。无需拆开模型。

一体模式仍可使用 BP_Rifle 的参数和事件；其原 WeaponMesh、Muzzle 相对偏移不再决定一体模型的枪口。独立枪械模式关闭 Use Integrated Weapon Mesh 后，继续使用 RifleAttachSocket 和 BP_Rifle.Muzzle。

## 3. 移动只保留 Idle 与 Jog

可以复制官方 Murdock_AnimBlueprint 作为项目 ABP，保留其通用 Pawn/Character 的移动数据读取和待机/起停/跳跃结构。Speed 使用 Get Velocity → Vector Length XY；Direction 保留官方相对角色朝向的角度计算。

移动 BlendSpace 使用 `/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/BlendSpaces/Murdock_Locomotion_BS`，其中已有 Idle 与 Jog_Fwd/Bwd/Left/Right_Alt。方向/速度连接对应轴，速度样本与实际 Jog 速度对齐。已有状态名 Run 可以保留或改名 Jog，决定动作的是内部动画引用。Sprint/Walk 的动画选择分支需在 ABP 中断开；单一 C++ 速度不会自动编辑二进制动画图。Idle/JogStop 起停过渡可以保留。

IA_Sprint 在 IMC 中的旧映射可以移除，C++ 已不消费它。原移动、视角、跳跃映射仍保留，避免重复建立第二套移动输入。

## 4. 左键有伤害但没有动作时

BP_MothPlayer 当前文件仍引用 Mannequin 的 AM_MothRifleFire / AM_MothRifleReload。Murdock 使用 Murdock_Skeleton，需要换成其自身的 Montage；不对整套 Murdock 模型/动画再做 Mannequin 重定向。

1. 从 `/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Fire_Fast_Montage` 复制项目版开火 Montage，指定到 BP_MothPlayer → Player → Animation → Fire Montage；也可以预览 Fire_NoReload 后创建 Montage。保留所选源动画的 Additive 设置。
2. 用户贴出的官方 AnimGraph 的 Slot 名是 `UpperBody`。开火 Montage 的 Slot 轨道也应为 `UpperBody`；若列表没有，在 Murdock_Skeleton 的 Window → Anim Slot Manager 创建该 Slot，并保存骨架。核对运行中真正使用的 ABP，而不是另一个未绑定的副本。
3. 官方演示角色的 Attacking、ResetCombo、ComboAttackSave 执行链先断开。实际扣弹与命中由现有 C++ 驱动，不能另外用输入/Notify 再执行一次。
4. Fire_Reload 是换弹表现候选，先预览其动作是否符合当前 R 键的整匣换弹，再创建 Murdock 骨架的 Reload Montage 并指定同名上半身 Slot。C++ 按既有 ReloadSeconds 调整播放速率，动画不负责补弹。
5. 无专用投掷动画时 Throw Montage 可留空；配置后也只负责表现，释放和 ThrowRecovery 使用既有 C++ 流程。

这是 hitscan 射击，弹药减少/目标 HealthComponent 扣血是逻辑证据，不要求先出现可见弹丸。枪口闪光/声音可接 BP_Rifle.OnShotFired；使用 Get Muzzle Location 取得当前一体枪口位置，在该位置生成反馈，不附着到已隐藏的 Rifle Actor。未配置 Niagara/声音资产不会取消真实命中。

## 5. 交互与其他表现迁移

E 拾取/放下、持物左键投掷、换弹取消、失焦/暂停松键门控和生命逻辑继续由 MothEffectCharacter / Rifle / HealthComponent 提供。Device Attach Socket 使用 Murdock 中实际存在的 `hand_r`，调整 DeviceHoldPoint 的相对 Transform。动画图从 Get Action State / Is Carrying Device / Get Held Device / Get Rifle 读取既有状态，选择 Carrying、ThrowRecovery、Reloading、Dead 的表现。

Rifle Actor 在一体模式始终隐藏，包括拾取后放下；不会恢复显示旧独立枪。Murdock 模型中自带的枪仍属于角色 Mesh，当前代码没有隐藏整个角色，也没有猜测枪的蒙皮骨骼。若需要携带时收枪，配置对应姿态；需要隐藏枪时先核对枪体实际蒙皮骨骼，再做骨骼/材质表现适配。

控制器 Mapping Contexts 必须包含 IMC_MothGameplay，其中 IA_Primary 对应 Left Mouse Button、IA_Reload 对应 R、IA_Aim 对应 Right Mouse Button、IA_Interact 对应 E。当前 BP_MothGameMode 文件引用的是 BP_ThirdPersonPlayerController；这套已有控制器或新 BP_MothPlayerController 均可用，须核对实际类的映射，不以文件夹名称判断是否已启用。

## 6. 本轮验证范围

助手未编译、未运行 PIE 或修改二进制资产。新增 [RifleMuzzleTests.cpp](../../Source/MothEffect/Private/Tests/RifleMuzzleTests.cpp) 两项 Automation 源码：`MothEffect.Weapons.Muzzle.ExternalSourceFollowsMovement` 与 `InvalidSourceFailsClosed`，验证外部来源动态位置、隐藏逻辑 Actor、缺失挂点/来源销毁拒绝回退、切回独立枪口。测试使用普通 SceneComponent，不替代实际 Skeletal Mesh Socket、动画刷新、完整射击或玩家速度的 PIE 验收；执行结果待用户提供。

本次配置后依次检查：

1. WASD、瞄准、Shift、开火、换弹、携带组合下，MaxWalkSpeed 始终与 JogSpeed 一致；静止仍为 Idle，移动为 Jog。
2. BP_Rifle 勾选 Draw Debug Shots，左键扣弹、停止连射、命中靶子扣血；移动/上下瞄准时红色射线起点随模型枪口移动，场景中不出现第二把枪。
3. 靠墙仍扣弹但墙后目标不受伤；枪口配置错误时不扣弹、不从角色中心射击；恢复合法配置后重进 PIE 测试。
4. 开火/换弹 Montage 使用 Murdock 骨架与实际上半身 Slot，移动腿部不被锁住；停用 Montage 后玩法仍正确结算。
5. 携带时不能射击/换弹；投掷后松键与恢复均完成才接受新开火。暂停/失焦停止连射，恢复不自动续射；死亡与重新进 PIE 不残留任务。

TC01、TC24、TC34、TC35、TC36、TC38 继续按 [测试计划](../06_Test_Doc/Test_Plan.md) 记录实际分支，不将旧模板结果自动扩展到 Murdock。
