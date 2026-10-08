# Moth Effect（飞蛾效应）：机关状态与命中接入

版本 v0.4 · 2026-10-08 · T06 当前三项 Automation 与 PIE 分支通过，待用户提交验收记录并走 PR 流程

对应 [T06 Issue #8](https://github.com/YYchainsAw/MothEffect/issues/8)。复用 T05 的 `FHitContext`、`IBallisticReactive` 与 Rifle 命中入口；用户已执行编译、Automation 和本轮 PIE 验收，助手仅核对及归档日志。接口与碰撞的权威定义见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)，规则见 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)，默认尺寸与质量对应 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 的 throw 组。

2026-10-08 T07 后续交付已写入实际 Held 拾取/附着/安全释放和玩家恢复源码，尚待用户编译、配置与验收；步骤见 [拾取、放下与安全投掷接入](Device_Interaction_Setup.md)。下文保留 T06 当轮的范围与通过记录，不能作为新增 T07 逻辑通过的证据。

2026-10-08 T08 行为更新：Emitter 种类现在会在命中后自动启动 D03，工作到期进入 Spent 并短暂残留后销毁；已有 `BP_DeviceBase` 若使用默认 Emitter 种类也会采用新行为。手动 J 结束请在 Active 工作期间调用；原 T06 的历史两次 J 结果不是新增寿命功能的验收。当前配置与重跑步骤见 [D03 与 P01 接入](Emitter_Setup.md)。

## 1. 本轮交付范围

- [DeviceTypes.h](../../Source/MothEffect/Public/Types/DeviceTypes.h) 定义机关状态与种类，独立于玩家行动状态。
- [DeviceBase.h](../../Source/MothEffect/Public/Devices/DeviceBase.h) / [DeviceBase.cpp](../../Source/MothEffect/Private/Devices/DeviceBase.cpp) 提供物理球根、独立射击判定球、无碰撞外观、状态事件、命中激活与结束清理。
- `TryActivate` 只接受已开始运行、未暂停、玩法许可的 Dormant。命中 ID 有效，命中点/世界弹道方向为有限数值、方向非零，伤害为有限非负数后，先提交 Active，再通知状态变化和调用原生效果入口；同一或不同 HitId 再次命中都不能重启，也不覆盖第一次命中的方向。
- 激活无需先扣光机关血量，也不要求命中伤害大于零。保存激活 HitId/方向/命中点，不长期持有命中来源 Actor。
- `FinishActivation` 一次进入 Spent，关闭碰撞和物理；再次结束或激活失败。`DestroyDevice` 和 EndPlay 最终进入 Destroyed；重复清理不会重复停止效果。
- `CanBePickedUp` 只在允许玩法的 Dormant 返回 true；Active、Spent、Destroyed 返回 false。它只表示机关端资格，T07 还要验证玩家行动、距离、遮挡和安全释放位置。
- Held 使用仅向 Character 开放的 `CommitHeld` / `CommitReleased`：合法拥有者才能进入 Held，只有该拥有者能释放回 Dormant；Held 关闭物理和两个判定体的碰撞。实际拾取输入、附着、玩家持有引用和安全释放在 T07 接入后实测。T06 不提供可在蓝图任意回写状态的 setter。
- `OnDeviceStateChanged` 用于蓝图表现；`OnDeviceStateChangedNative` 用于 C++ 观察者。回调可能触发进一步结束/销毁，读取当前状态使用 `GetDeviceState`；再次命中不能在回调中重复激活。

本轮的原生效果入口为空，便于先验证状态与命中。D03 发射属于 T08，D02/D01 属于 T10/T11；完整容量、闲置过期与计数属于 T13。本轮没有让空效果装置自动结束，也不表示三机关已实现。

## 2. 用户编译与灰盒资产

1. 退出 PIE，Save All；关闭编辑器后，用现有 IDE 编译 `MothEffectEditor / Development Editor / Win64`，再打开工程。新增反射类与枚举按完整编译接入；助手不代为编译。
2. 在 `/Game/MothEffect/Devices` 创建 **BP_DeviceBase**，父类选择 **DeviceBase**。
3. Components 中选 **DeviceMesh**，设置一个 Sphere 网格，调整可见尺寸与 PhysicsBody 一致。引擎 BasicShapes 可作为占位；必要时在内容浏览器显示 Engine Content。Actor Scale 保持 1，外观只调整 DeviceMesh 的 Transform。
4. 保留继承的 **PhysicsBody** 为根，**ShotCollider** 为命中判定。DeviceMesh 无碰撞；Held/Spent 的碰撞由 C++ 状态控制，不给外观网格另开 BlockAll。
5. Class Defaults 中保留当前 **Weapon Trace Channel=Visibility**，与 BP_Rifle 一致；勾选 **Log Device Events**。当前正式 JamDevice/JamWeaponTrace 矩阵仍待集中接入，此处沿用武器验证房的查询通道。
6. 放一个 BP_DeviceBase 到测试房地面上方，保存关卡并进入 PIE。无需给它添加 Health Component 或在蓝图重新实现 Ballistic Reactive 接口。

## 3. T06 局部验收

1. 机关落地受重力，普通碰撞可推动，不会因此启动。
2. 用步枪射击一次；Output Log 中出现唯一 `Dormant -> Active`，命中日志包含 HitId、source、instigator、point、direction、damage 与 accepted。
3. 继续单发和连射；后续命中 `accepted=0`，没有第二次 Active 转换，首次 HitId/方向保持不变。Active 仍阻挡武器射线且保持物理运动。
4. 在测试关卡蓝图临时用按键获取这个 BP_DeviceBase 并调用 `Finish Activation`；第一次返回 true，进入 Spent；第二次返回 false。Spent 无碰撞，继续射击不激活。
5. 调用 `Destroy Device` 或销毁 Actor，核对 Destroyed 日志与清理；重新进入 PIE 得到新的 Dormant。暂停中不接受手动测试命中；`Set Gameplay Enabled(false)` 拒绝激活，并结束正在工作的效果。

对应 TC12 的当前机关壳分支。Held 禁止激活及拾取/放下循环的实际操作待 T07；D03 持续时间与方向反馈待 T08；效果结束计数/过期待 T13。记录实际构建、环境、参数差异、步骤、结果与日志后再推进验收状态。

## 4. 自动化验证入口

[DeviceActivationTests.cpp](../../Source/MothEffect/Private/Tests/DeviceActivationTests.cpp) 提供以下测试，每个测试创建并清理独立 World：

| 测试名（前缀 `MothEffect.Devices.`） | 验证内容 |
|---|---|
| `ActivationIsSingleUse` | 接口接受零伤害命中；连续和回调内重复命中仅一次 Active；首次方向/HitId/命中点锁定；Active/Spent 不可拾取；完成和销毁只广播一次 |
| `RejectInvalidAndDisabledHits` | 无效 HitId、零方向、非有限点/方向/伤害被拒绝；暂停与玩法禁用拒绝命中；被拒绝的 Dormant 之后仍可合法激活；Spent 不复活 |
| `CancelDuringActivationCallback` | Active 通知期间取消玩法立即进入 Spent；返回后不重新开启物理，也不能恢复激活资格 |

在 Automation 面板搜索 `MothEffect.Devices` 运行。用户的修正后 Test Run 4 三项均通过；修正前的失败记录见第 6 节，成功证据见第 7 节。助手未代为编译或执行 UE。

## 5. 当前验证记录

- 工作分支：`develop`；用户已提交 T06 基础和修正，当前源码 HEAD 为 `c0a7589ccc78003069d247037c4b7fa2daca5971`。测试执行时为 `8085801` 加头文件路径修正，该修正现已包含于 `c0a7589`；验收日志摘录和本轮文档同步仍待用户提交。
- 玩法参数文档：v0.9；机关默认物理尺寸、命中球半径和质量对应 throw 组。本轮未调整参数；玩家移动速度的既有差异见 Player_Setup。
- 用户实际结果：构建检查成功；三项 Automation 全部通过；PIE 首次命中激活、重复命中不重启、J 键结束到 Spent、再按 J 返回 false 均通过。依据与时间见第 7 节。
- 助手验证范围：UE 5.8 本机接口/生命周期源码、既有构建/测试日志和仓库静态检查。本轮未代为编译、执行测试或创建 PR。
- T06 当前状态/命中壳验收通过，等待用户提交记录并完成 PR 流程；GitHub Issue/Project 不由本次文档同步自动修改。完整 TC12 随 T07/T08/T13 补测。

## 6. 2026-10-07 Automation Test Run 3 与修正

用户提供编辑器 Automation 结果，本机 `Saved/Logs/MothEffect.log` 的 07:21:19–07:21:20 UTC 记录与之相符：

| 测试 | 修正前实际结果 |
|---|---|
| `ActivationIsSingleUse` | 失败：接口首次调用未激活，之后直接 TryActivate 才激活，导致五条断言失败；销毁时有 World has no context 警告 |
| `CancelDuringActivationCallback` | 通过（用户执行） |
| `RejectInvalidAndDisabledHits` | 通过（用户执行） |

原因已通过 UE 5.8 源码和本机 UHT 生成代码定位：`Execute_ReceiveBallisticHit` 通过 `AActor::ProcessEvent` 调用接口，后者要求 World 已初始化 Actor。原测试仅对机关调用 `DispatchBeginPlay`，未调用 World 的 `InitializeActorsForPlay`，也未注册 World Context；直接 C++ 调用不经过该分发检查，因此其余测试可通过。

本轮仅修正测试环境：创建 World 后注册对应 Context，并调用 `InitializeActorsForPlay`；清理时先给已开始运行的 Actor 发送 EndPlay，再销毁 World 并注销 Context。保留接口 Execute 调用，并增加 World 初始化前置断言；首次接口激活失败即停止后续依赖断言，避免连带报错。

当次修正交付后等待用户重新编译、运行全部三项 `MothEffect.Devices`；后续复测已完成，结果见第 7 节。

同日 15:29（香港时间）用户重新编译失败，MSB3073 / code 6 的底层原因已由 UBT 日志确认：`DeviceActivationTests.cpp` 报 C1083，无法找到本轮误写的 `Engine/URL.h`。已改为本机 UE 5.8 中定义 FURL 的 `Engine/EngineBaseTypes.h`，并核对测试文件全部 9 个引号 include 都能解析到项目/引擎头文件；之后用户构建检查与 Automation 复测成功，见第 7 节。

## 7. 通过结果与证据

执行人：YYchainsAw；引擎：UE 5.8.0-55116800；环境：Windows 编辑器 Automation 与 PIE；玩法参数文档 v0.9，玩家既有参数差异见 Player_Setup。本轮验收记录于 2026-10-08 同步。

| 检查 | 实际结果与来源 |
|---|---|
| `MothEffectEditor Win64 Development` 构建检查 | 2026-10-07 15:32 香港时间，UBT 显示 Target is up to date / Result: Succeeded；该日志不表示干净重建 |
| `MothEffect.Devices` 三项 Automation | 2026-10-07 15:38:37 香港时间，Test Run 4 三项 Result={Success}；该轮未再出现 World has no context |
| PIE 首次与重复步枪命中 | 用户明确确认通过；日志可见一次 Dormant -> Active、首次 accepted=1、后续 accepted=0 |
| PIE J 键结束 | 用户明确确认通过；日志记录 Active -> Spent，第一次打印 true，第二次打印 false |

可提交的原始日志摘录及构建/源码标识见 [T06_Verification_2026-10-07.log](../06_Test_Doc/Evidence/T06_Verification_2026-10-07.log)。原 Saved/Logs 与 UBT 日志仍保留在用户本机；摘录移入 Docs 后可随提交共享。

待后续任务验收：T07 的 Held 拾取/释放实际操作、T08 的 D03 发射次数和工作寿命、T13 的完整容量/过期/计数及独立包回归。本轮结果不将完整 TC12 自动标记为通过。
