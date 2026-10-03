# Moth Effect（飞蛾效应）：玩家系统第一步接入

版本 v0.1 · 2026-10-03 · T04 进行中 · 修改后未编译、未运行验收

协作方式已经用户确认：助手修改 C++，用户在 UE 中配置资产与试玩验收。用户明确要求助手不代为编译，后续编译由用户自行执行。本次交付是越肩相机、镜头水平朝向、瞄准/冲刺的 C++ 基础与下列配置步骤；输入资产、Rifle 动画图、枪械与机关尚未接入。

## 1. C++ 已写入的内容

- [MothEffectCharacter.h](../../Source/MothEffect/Public/Characters/MothEffectCharacter.h) / [MothEffectCharacter.cpp](../../Source/MothEffect/Private/Characters/MothEffectCharacter.cpp)：越肩相机配置、镜头 Yaw 朝向、移动/跳跃参数、瞄准 FOV 混合、瞄准/冲刺互斥与蓝图可读状态。
- [MothEffectPlayerController.h](../../Source/MothEffect/Public/Framework/MothEffectPlayerController.h) / [MothEffectPlayerController.cpp](../../Source/MothEffect/Private/Framework/MothEffectPlayerController.cpp)：FlushPressedKeys 时清理瞄准/冲刺/持续跳跃状态。
- 保留原有 Move/Look/MouseLook/Jump 动作与控制器输入映射；新增 AimAction、SprintAction 需要在编辑器指定资产。数值按 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 初始化，未实测。

本次新增 UPROPERTY/UFUNCTION 与原生默认值。用户保存当前工作、关闭 UE 后自行完成 C++ 编译，再重新打开工程进行以下配置；不要把旧编辑器会话或 Live Coding 中可见的旧字段当作新代码已经加载。助手没有编译修改后的代码，静态检查不能替代编译成功。

## 2. 在 UE 创建输入资产

在 Content Browser 的 `/Game/MothEffect/Player/Input` 创建：

| 资产 | 类型与设置 | 按键 |
|---|---|---|
| IA_Aim | Input Action；Value Type 为 Digital/Bool；Triggers、Modifiers 留空 | 在映射上下文中指定鼠标右键 |
| IA_Sprint | Input Action；Value Type 为 Digital/Bool；Triggers、Modifiers 留空 | 在映射上下文中指定 Left Shift |
| IMC_MothGameplay | Input Mapping Context；添加上述两个动作映射 | Right Mouse Button / Left Shift |

此阶段只创建 Aim/Sprint；IA_Primary/Reload/Interact 随后续枪械与机关接入。移动、视角和跳跃继续使用现有 `/Game/Input`，不重新映射到新上下文。C++ 已绑定 Started 与 Completed/Canceled，UE 不需要再写一套瞄准或冲刺输入事件。[Epic：Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)

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

射击取消冲刺的入口 CancelSprintUntilRelease 已准备，但当前没有 ARifle，不能把“开火取消冲刺”记为通过。输入与相机配置完成后，下一步在 UE 制作 ABP_MothPlayer、八方向 Rifle BlendSpace 与腾空状态机；然后进入 T05 枪械/换弹/血量。

## 5. 动画接入时使用的状态

AnimBP 从角色读取 IsAiming、IsSprinting、AimBlendAlpha，从 CharacterMovement/角色速度读取 IsFalling、竖直速度与角色局部水平速度。基础八方向移动与腾空转换按照 [技术设计第 3.4 节](../02_Design_Doc/TDD/Technical_Design.md#34-动画资产与图结构) 实现；角色 Mesh 的 Anim Class、BlendSpace、Slot 与 Montage 由用户在 UE 配置，本轮未创建或修改这些二进制资产。

移动状态与瞄准已经有 C++ 数据来源；Ready/Carrying/ThrowRecovery/Reloading/Dead 的完整行动状态机随 T05–T09 实现。G0/G1 仍需各自的完整验收，进度与证据归入 [开发计划](Development_Plan.md) 和 [测试计划](../06_Test_Doc/Test_Plan.md)。
