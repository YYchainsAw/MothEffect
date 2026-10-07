# Moth Effect（飞蛾效应）：机关状态与命中接入

版本 v0.2 · 2026-10-07 · 用户已运行 Automation；测试 World 已修正，待重新编译和复测

对应 [T06 Issue #8](https://github.com/YYchainsAw/MothEffect/issues/8)。复用 T05 的 `FHitContext`、`IBallisticReactive` 与 Rifle 命中入口；本轮未编译或运行 UE。接口与碰撞的权威定义见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)，规则见 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)，默认尺寸与质量对应 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 的 throw 组。

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

用户完成编译后，可在 Automation 面板搜索 `MothEffect.Devices` 运行。用户已执行修正前的三项测试，结果见第 6 节；本轮修正后的测试待重新编译和复测。助手未代为编译或执行 UE。

## 5. 当前验证记录

- 工作分支：`develop`；用户已提交 T06 基础，当前源码基线 HEAD 为 `3b0e93f1fa4c2369e466139acdf3c39b308da44c`。本轮测试 World 修正尚未提交；复测记录还需注明包含该工作区修改及实际编译标识。
- 玩法参数文档：v0.9；机关默认物理尺寸、命中球半径和质量对应 throw 组。本轮未调整参数；玩家移动速度的既有差异见 Player_Setup。
- 助手验证范围：UE 5.8 本机接口/生命周期源码核对及仓库静态检查。本轮修正后的编译、三项 Automation 复测、PIE 步枪命中和状态日志仍待用户执行。
- [Issue #8](https://github.com/YYchainsAw/MothEffect/issues/8) 仍为 Open / In Progress。完成编译与本轮 PIE 分支并留下记录后，再按实际验收推进；完整 TC12 随 T07/T08/T13 补测。

## 6. 2026-10-07 Automation Test Run 3 与修正

用户提供编辑器 Automation 结果，本机 `Saved/Logs/MothEffect.log` 的 07:21:19–07:21:20 UTC 记录与之相符：

| 测试 | 修正前实际结果 |
|---|---|
| `ActivationIsSingleUse` | 失败：接口首次调用未激活，之后直接 TryActivate 才激活，导致五条断言失败；销毁时有 World has no context 警告 |
| `CancelDuringActivationCallback` | 通过（用户执行） |
| `RejectInvalidAndDisabledHits` | 通过（用户执行） |

原因已通过 UE 5.8 源码和本机 UHT 生成代码定位：`Execute_ReceiveBallisticHit` 通过 `AActor::ProcessEvent` 调用接口，后者要求 World 已初始化 Actor。原测试仅对机关调用 `DispatchBeginPlay`，未调用 World 的 `InitializeActorsForPlay`，也未注册 World Context；直接 C++ 调用不经过该分发检查，因此其余测试可通过。

本轮仅修正测试环境：创建 World 后注册对应 Context，并调用 `InitializeActorsForPlay`；清理时先给已开始运行的 Actor 发送 EndPlay，再销毁 World 并注销 Context。保留接口 Execute 调用，并增加 World 初始化前置断言；首次接口激活失败即停止后续依赖断言，避免连带报错。

待用户保存并关闭编辑器、重新编译后，再运行全部三项 `MothEffect.Devices`。修正后的结果目前为待执行；既有两个通过结果不自动替代修改后的复测。
