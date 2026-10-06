# Moth Effect（飞蛾效应）：玩家输入与动画接入

版本 v0.4 · 2026-10-06 · T04 Issue 已关闭 / Project 为 Pending Acceptance · 基础项已勾选，完整分支证据待补

协作方式已经用户确认：助手修改 C++，用户在 UE 中配置资产与试玩验收。用户明确要求助手不代为编译，后续编译由用户自行执行。C++ 已写入越肩相机、镜头水平朝向、瞄准/冲刺；本轮复用现有读取接口，继续接入动画图。

用户已报告创建 IA_Aim、IA_Sprint、IMC_MothGameplay，从 ABP_Unarmed 复制 ABP_MothPlayer，制作 Rifle 八方向 BlendSpace，并在 ABP_MothPlayer 创建 IsAiming 变量。已静态核对资产存在并阅读用户粘贴的节点文本；用户随后报告当前移动/瞄准动画测试成功。2026-10-06 的 [T04 Issue #6](https://github.com/YYchainsAw/MothEffect/issues/6) 已关闭、四项基础验收已勾选，但 Project 仍为 Pending Acceptance，记录字段待填。助手没有运行 PIE，不把局部报告或勾选扩展为完整 TC34/TC35 通过；下一步见 [步枪、换弹与生命接入](Weapon_Setup.md)。

## 1. C++ 已写入的内容

- [MothEffectCharacter.h](../../Source/MothEffect/Public/Characters/MothEffectCharacter.h) / [MothEffectCharacter.cpp](../../Source/MothEffect/Private/Characters/MothEffectCharacter.cpp)：越肩相机配置、镜头 Yaw 朝向、移动/跳跃参数、瞄准 FOV 混合、瞄准/冲刺互斥与蓝图可读状态。
- [MothEffectPlayerController.h](../../Source/MothEffect/Public/Framework/MothEffectPlayerController.h) / [MothEffectPlayerController.cpp](../../Source/MothEffect/Private/Framework/MothEffectPlayerController.cpp)：FlushPressedKeys 时清理瞄准/冲刺/持续跳跃状态。
- 保留原有 Move/Look/MouseLook/Jump 动作与控制器输入映射；新增 AimAction、SprintAction 在编辑器指定。用户此前要求只改代码，WalkSpeed/AimMoveSpeed 已改为 300、SprintSpeed 改为 625，动画由用户调整；[玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 仍保留旧移动速度，后续调参归档时需同步，不用旧账本值覆盖当前代码/动画。其余起步数值沿用账本。

本次新增 UPROPERTY/UFUNCTION 与原生默认值。用户保存当前工作、关闭 UE 后自行完成 C++ 编译，再重新打开工程进行以下配置；不要把旧编辑器会话或 Live Coding 中可见的旧字段当作新代码已经加载。助手没有编译修改后的代码，静态检查不能替代编译成功。

## 2. 在 UE 创建输入资产

用户已将输入资产放在 `/Game/MothEffect/Input/Actions` 和 `/Game/MothEffect/Input/Mapping`，沿用这些实际路径：

| 资产 | 类型与设置 | 按键 |
|---|---|---|
| IA_Aim | Input Action；Value Type 为 Digital/Bool；Triggers、Modifiers 留空 | 在映射上下文中指定鼠标右键 |
| IA_Sprint | Input Action；Value Type 为 Digital/Bool；Triggers、Modifiers 留空 | 在映射上下文中指定 Left Shift |
| IMC_MothGameplay | Input Mapping Context；添加上述两个动作映射 | Right Mouse Button / Left Shift |

本页对应 Aim/Sprint；IA_Primary/IA_Reload 按 Weapon_Setup 接入，IA_Interact 随机关接入。移动、视角和跳跃继续使用现有 `/Game/Input`，不重新映射到新上下文。C++ 已绑定 Started 与 Completed/Canceled，UE 不需要再写一套瞄准或冲刺输入事件。[Epic：Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)

## 3. 给现有角色和控制器赋值

1. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`，在 Class Defaults 的 Input 中将 **Aim Action** 指定为 IA_Aim、**Sprint Action** 指定为 IA_Sprint；保留原 Move/Look/MouseLook/Jump 引用。
2. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController`，在 Class Defaults 的 Input → Input Mappings 中，把 IMC_MothGameplay **追加**到 Default Mapping Contexts；保留现有 Default/MouseLook 所在数组的已有条目。
3. 先使用 `/Game/ThirdPerson/Lvl_ThirdPerson`，确认实际 GameMode、Default Pawn Class 和 Player Controller Class 仍使用标准 ThirdPerson 的这套蓝图，而不是模板 Combat/Platforming/SideScrolling 变体。
4. 在角色 Class Defaults 的 Player → Movement / Camera 检查新的配置字段。运行时 BeginPlay 会以这些字段应用角色朝向、相机臂/偏移与移动参数；调参修改这些字段并同步参数账本，不在旧 CharacterMovement/CameraBoom 字段另维护第二套数值。
5. 保存蓝图和输入资产，进入 PIE 验证。缺少 AimAction/SprintAction 会有一次配置日志；若按键无效，先核对动作引用与控制器映射是否都已完成。

## 4. 第一轮手动验收

先做 TC34 的基础分支，记录结果，不自动把整个 T04 或 TC34 标为通过：

1. WASD/鼠标/Space 仍可用；角色跟随镜头水平朝向，镜头向上/下看不把角色整体倾斜。
2. 按住右键，FOV 平滑进入瞄准；松开恢复。可在 PIE 调试角色中读取 IsAiming、AimBlendAlpha 和 CharacterMovement.MaxWalkSpeed，与参数账本核对。
3. 按住 Shift 移动可冲刺；冲刺时按右键立即取消冲刺。保持 Shift、松开右键仍是普通移动，松开再按 Shift 才再次冲刺。
4. 先按住右键，再按 Shift，不进入冲刺；松开右键也不自动冲刺，需要重新按 Shift。
5. 保持瞄准/冲刺后切换窗口，回到游戏核对状态已清理；新输入可正常工作。
6. 靠墙移动与转镜头，检查 SpringArm 遮挡与角色是否持续挡住屏幕中心。这里只验收镜头，枪口遮挡与安全投掷随 T05/T07 执行。

T05 已写入 ARifle 并调用 CancelSprintUntilRelease，实际“开火取消冲刺”仍须配置武器后验收。输入与相机配置完成后，按第 5 节连接 ABP_MothPlayer、八方向 Rifle BlendSpace 与腾空状态机；然后按 Weapon_Setup 接入枪械/换弹/血量。

## 5. 连接 ABP_MothPlayer

本地资产为 `/Game/MothEffect/Animations/Player/ABP_MothPlayer` 与 `/Game/MothEffect/Animations/Player/Combat/BS_MothRifleLocomotion`。沿用内建 UAnimInstance 父类，角色与移动组件提供只读数据，用户在 UE 配置动画图；本轮助手没有修改二进制资产或执行编译。

### 5.1 把 IsAiming 变量连接到角色

打开 ABP_MothPlayer 的 Event Graph。确认 IsAiming 为 Boolean，默认 false。使用普通 Event Blueprint Update Animation，每帧读取现有 C++ 角色：

~~~text
执行引脚：Event Blueprint Update Animation → Cast To MothEffectCharacter → Set IsAiming
数据引脚：Try Get Pawn Owner.Return Value → Cast.Object
          Cast.As MothEffectCharacter → Is Aiming.Target
          Is Aiming.Return Value → Set IsAiming.Value
~~~

从 Cast 成功后的角色对象引脚拖出线，搜索 Is Aiming，选择 Target 为 MothEffectCharacter 的只读函数；然后将 ABP 自己的 IsAiming 拖入图表并选择 Set。角色的函数与 ABP 的变量可以同名，Target 决定读的是哪一份数据。Cast Failed 可连接 Set IsAiming=false，避免无有效 Pawn 时继续显示旧状态。

如果 Event Graph 已有 Event Blueprint Update Animation，从已有执行链追加或用 Sequence 分支；保留模板仍使用的初始化与移动数据读取。不要为同一个事件另起重复实现。当前 C++ getter 未标记 BlueprintThreadSafe，这些普通对象调用放在 Event Graph；复制模板若已有 Blueprint Thread Safe Update Animation，检查它是否也写入同名变量，每个变量保留一个更新来源。[Epic：动画变量读取](https://dev.epicgames.com/documentation/unreal-engine/how-to-get-animation-variables-in-animation-blueprints-in-unreal-engine?lang=zh-CN)；[Epic：动画更新与线程](https://dev.epicgames.com/documentation/unreal-engine/animation-optimization-in-unreal-engine)

先进入 PIE，再在 ABP 顶部 Debug Filter 选择实际玩家实例，观察右键按住为 true、松开为 false。动画编辑器独立预览可能没有 Pawn Owner，Cast 失败时按默认 false 处理，不用 Print String 每帧刷屏。

### 5.2 补齐移动与瞄准数据

以下是动画变量，可复用模板已有的同类变量；保留实际名称，避免建立两份 Speed / GroundSpeed。均在角色 Cast 成功后读取；角色无效时将这些动画快照复位。

| ABP 变量 | 类型 | 数据来源 |
|---|---|---|
| IsAiming | Boolean | 角色 Is Aiming |
| IsSprinting | Boolean | 角色 Is Sprinting |
| AimBlendAlpha | Float | 角色 Get Aim Blend Alpha；已有 C++ 平滑，不另做第二次插值 |
| Velocity | Vector | 角色 Get Velocity |
| GroundSpeed | Float | Velocity → Vector Length XY；只计算水平速度 |
| Direction | Float | Calculate Direction：Velocity 接角色速度，Base Rotation 接角色 Get Actor Rotation |
| IsFalling | Boolean | 角色 Get Character Movement → Is Falling |
| VerticalSpeed | Float | Velocity 的世界 Z |
| AimPitch | Float | 角色 Get Base Aim Rotation 与 Get Actor Rotation → Delta (Rotator)，取 Pitch；当前 AO_Rifle 轴为 -1～1，将角度按下文归一化 |

Calculate Direction 输出的是相对角色朝向的水平角度；前向约 0°、右侧约 90°、左侧约 -90°、后退约 ±180°。这是已确定的局部水平速度表达的角度/速度形式。变量的普通对象调用在 Event Graph 完成，AnimGraph 使用这些快照。[Epic：CalculateDirection](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/AnimGraphRuntime/UKismetAnimationLibrary/CalculateDirection)

### 5.3 接入现有 Rifle BlendSpace

1. 在角色 BP 的 Mesh → Animation 中设 Animation Mode=Use Animation Blueprint、Anim Class=ABP_MothPlayer。
2. 打开 BS_MothRifleLocomotion 的 Asset Details，确认哪一轴是 direction、哪一轴是 Speed。方向单位为度，速度单位为 cm/s；速度轴应覆盖参数账本里的冲刺速度，样本位置要对应各自实际 Walk/Jog 速度。资产中的轴顺序以编辑器为准。
3. 在 ABP 的 AnimGraph 中找到真正连到输出的地面移动状态。将其 BlendSpace Player 资产替换为 BS_MothRifleLocomotion，direction 输入接 Direction，Speed 输入接 GroundSpeed。
4. 确认零速待机最终使用 Rifle 持枪待机。若模板有单独 Idle 状态，同步将其序列替换为 MF_Rifle_Idle_ADS；如果 BlendSpace 零速区域已经覆盖持枪待机，地面状态可以直接由该 BlendSpace 输出。
5. 地面姿态仍输出到现有最终动画链。逐项验证 W/S/A/D 与四个斜向；方向颠倒先检查 Calculate Direction 的 Base Rotation 和样本位置。

如沿用模板 ShouldMove 决定待机/移动，使用实际水平速度作为依据，外力推动时也应能进入移动表现。首版位移由 CharacterMovement 控制，引用序列核对 Root Motion 设置与骨架兼容。

### 5.4 先验证基础腾空，再细分状态

T04 最低接入先使用 Grounded 与 InAir，完成后再细分技术设计规定的 JumpStart / InAir / Land；T04 的 Issue 状态与完整专项测试分别记录：

| 转换 | 条件 |
|---|---|
| Grounded → InAir | IsFalling |
| InAir → Grounded | NOT IsFalling |

InAir 内使用 Blend Poses by Bool：VerticalSpeed >= 0 时播放 MM_Rifle_Jump_Start_Loop，下降时播放 MM_Rifle_Jump_Fall_Loop，循环均启用。这样跳跃、走落边缘与外力腾空共用移动组件判据。后续 JumpStart 需要区分主动跳跃与外力进入，Land 需要允许再次腾空中断；恢复动画 MM_Rifle_Jump_RecoveryAdditive 不能当作普通全身序列使用。

### 5.5 接入俯仰瞄准

先验证 IsAiming 的数据变化，再在 AnimGraph 使用模板 AO_Rifle。AO 是 Additive 姿态，需要正常持枪姿态作为 Base Pose；模板 AO 先在编辑器核对骨架、Mesh Space Additive 与样本基准姿态。[Epic：Aim Offset](https://dev.epicgames.com/documentation/unreal-engine/aim-offset-in-unreal-engine?lang=en-US)

~~~text
移动状态机 → Save Cached Pose（MothLocomotion）
Use Cached Pose（MothLocomotion）──────────────────────────────→ Two Way Blend.A
Use Cached Pose（MothLocomotion）→ AO_Rifle.Base Pose → AO输出 → Two Way Blend.B
AimBlendAlpha ───────────────────────────────────────────────→ Two Way Blend.Alpha
Two Way Blend → 现有最终姿态链 → Output Pose
~~~

用户确认 AO_Rifle 的轴为 -1～1。AimPitch 使用：`视角 Pitch - 角色 Pitch → Normalize Axis → / 90 → Clamp(-1, 1)`；如已使用 Delta (Rotator)，其 Pitch 已标准化，再除以 90 并 Clamp。不要直接把角度 Clamp 到 -1～1，否则超过一度就落到端点。预览验证上下方向，若资产符号相反再取负。

AO_Rifle 的俯仰轴输入 AimPitch，另一轴先为 0，因为当前身体已经跟随镜头 Yaw；具体 X/Y 顺序在 AO 资产中检查。若轴仅名为 X/Y，先在资产预览中移动轴，确认哪个轴控制上下瞄准。使用 Two Way Blend，Alpha Input Type 设为 Float；Alpha 在 0 时使用 A、1 时使用 B。AimBlendAlpha 同时驱动相机与动画平滑，右键的 IsAiming 用于调试/条件判断。

AO 节点已经把 Additive 应用于 Base Pose，其输出按普通最终姿态参加混合。先保留模板末端仍需要的 IK/Control Rig，并检查它们是否影响持枪；开火/换弹的 UpperBody Slot 与 Layered Blend per Bone 随 T05 接入。

### 5.6 本轮手动检查

- PIE 的实际 ABP 实例中，IsAiming 和 AimBlendAlpha 随右键变化，松开后恢复。
- W/S/A/D 与斜向使用正确 Rifle 移动；冲刺时 GroundSpeed 与移动组件一致。
- 移动中瞄准，腿部仍由移动状态机驱动；向上/下看改变持枪俯仰，身体不整体倾斜。
- 跳跃和走落边缘都进入 InAir，落地恢复 Grounded；无持续 Accessed None 或线程安全调用警告。
- 用户已报告当前移动/瞄准动画测试成功；仍需记录实际执行的 TC34/TC35 分支，助手未运行 PIE，完整案例测试表不因此改为通过。

移动与瞄准已有 C++ 数据来源，T05 已加入 Ready/Reloading/Dead；Carrying/ThrowRecovery 随 T06–T09 实现。G0/G1 仍需各自的完整验收，进度与证据归入 [开发计划](Development_Plan.md) 和 [测试计划](../06_Test_Doc/Test_Plan.md)。
