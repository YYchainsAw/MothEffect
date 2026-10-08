# Moth Effect（飞蛾效应）：T07 拾取、放下与安全投掷接入

版本 v0.1 · 2026-10-08 · 状态：C++ 与测试源码已写入，待用户编译、配置资产和验收

对应 [T07 / Issue #9](https://github.com/YYchainsAw/MothEffect/issues/9)，依赖 T04 玩家与 T06 装置壳。继续在已有 `develop` 开发。助手未编译、未启动 UE、未运行 Automation/PIE；本轮未创建分支或 PR。规则依据是 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) R04/R13/R14/R18/R21，数值依据是 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) v0.9 的 throw 组。

## 1. 已写入的行为

- E 在 Ready/Reloading 时检测准星第一个阻挡命中，使用装置的射击球作为候选；验证 Dormant、角色距离、相机与角色胸前到装置的无遮挡路径、单持有及有效手部挂点。只有全部成功才取消连射/未完成换弹，进入 Carrying；失败继续原换弹。
- Held 时 E 安全放下，左键 Started 只尝试一次投掷；R 与步枪开火被拒绝。移动、跳跃、右键转向瞄准及非瞄准冲刺仍可用。
- `DeviceHoldPoint` 随手部骨骼/Socket，组件 Transform 可在角色 BP 调整。Held 停物理并关闭 PhysicsBody、ShotCollider、DeviceMesh 碰撞，步枪 Actor 隐藏。射击查询与激活接口都不会把持物变成护盾，玩家血量仍按命中接口处理。
- 放下/投掷共用与实际缩放物理球一致的球体 Sweep、起点和终点重叠检测。先检测角色胸前到身体外侧的短路径，再检测外侧到释放点，避免薄墙恰好位于起点之前；只忽略自身、所持道具及自己的步枪，其他 Pawn/物理物件仍阻挡。
- 释放点由角色位置加镜头水平前向偏移及世界 Z 偏移构成，不从摄像机位置生成。投掷与步枪共用 `ARifle::GetAimTarget` 的相机目标，从释放点朝目标取得方向，再叠加世界 Z 的向上速度；放下初速度为零。
- 成功释放先 Detach、回 Dormant、启用重力/物理、写入速度并清除双向持有引用，再通知表现。放下直接 Ready；投掷进入 ThrowRecovery，由世界 Timer 完成，暂停冻结剩余时间。恢复结束只允许接收新请求，不自动开火。
- 投掷成功要求实际松左键；长按或恢复期间提前按下均不缓存射击。受阻保持 Held，按压仍归属 Throw，松键后才可再次尝试。输入 Canceled/Flush 不作为实际松键。
- 死亡、结算禁用、EndPlay 取消恢复并销毁 Held，不额外掉落。所持道具意外销毁/结束运行或禁用时，角色清引用、恢复枪的可见性，并要求左键实际释放；存活的 Carrying 返回 Ready。
- `ThrowMontage`、`OnDeviceReleased`、`OnDeviceInteractionRejected` 只控制反馈。物理释放与恢复不依赖 AnimNotify 或 Montage 播完。

本轮没有实现 D03 发射/P01、D01/D02、容量和闲置寿命，也未改正式碰撞通道。继续沿用当前 Visibility 射击通道；安全释放使用物理根的对象类型与碰撞响应。参数仍由 C++ 默认值/BP 配置维护，没有新增 JSON 运行时导入器。

实现：[角色交互](../../Source/MothEffect/Private/Characters/MothEffectCharacterDevices.cpp)、[释放安全查询](../../Source/MothEffect/Private/Devices/DeviceReleaseSafety.cpp)、[装置基类](../../Source/MothEffect/Private/Devices/DeviceBase.cpp)、[碰撞测试](../../Source/MothEffect/Private/Tests/DeviceReleaseSafetyTests.cpp)。

## 2. 编译与 E 输入

1. 退出 PIE，Save All，关闭编辑器。在现有 IDE 编译 `MothEffectEditor / Development Editor / Win64`，成功后重新打开项目。新增反射字段需要完整编译，不用 Live Coding 代替。
2. 在 `/Game/MothEffect/Input/Actions` 新建 **Input Action：IA_Interact**，Value Type 设 **Digital (bool)**，先不添加 Hold/其他 Trigger。
3. 打开已有 **IMC_MothGameplay**，新增 IA_Interact 映射 **E**。不要在角色 Event Graph 再加一套 E 拾取/放下逻辑。
4. 打开 **BP_ThirdPersonCharacter → Class Defaults → Input**，将 **Interact Action** 设为 IA_Interact。既有 Primary Action/Reload Action 保持当前配置。
5. Compile、Save。

## 3. 手部挂点与参数

1. 角色 **Player → Devices → Device Attach Socket** 默认 `hand_r`，可先直接挂右手骨骼。需要专门偏移时，在实际使用的 Skeleton 的 `hand_r` 下 Add Socket，命名 `DeviceSocket`，再将该字段改为 `DeviceSocket`。
2. 在角色 Components 选 **DeviceHoldPoint**，调整相对位置/旋转，让占位球中心落在掌前；C++ 不覆盖组件 Transform。用 PIE 实际拾取后的外观核对，必要时结束 PIE 再调整并保存。
3. 复用已创建的 **BP_DeviceBase**（当前资产 `/Game/MothEffect/BP_DeviceBase`），保留物理球根、独立射击球和无碰撞球形外观，Actor Scale 保持 1，只调整 DeviceMesh 外观大小。不要再生成一个专门的 Held 副本。
4. 保留 C++ 默认投掷字段，检查已有 BP 是否覆盖了它们；字段与账本对应如下。修改玩法数值时同步账本及决策，不把本表当第二份参数基线。

| 角色字段 | throw 参数 |
|---|---|
| Pickup Range Cm | pickupRangeCm |
| Throw Forward Velocity | forwardVelocityCmPerSec |
| Throw Upward Velocity | upwardVelocityCmPerSec |
| Device Release Forward Offset | spawnForwardOffsetCm |
| Device Release Up Offset | spawnUpOffsetCm |
| Min Gun Recovery Seconds | minGunRecoverySeconds |

释放前向距离必须容纳角色胶囊半径、实际物理球半径和 1 cm 净空；体积/偏移配置不满足时拒绝释放。这个净空是几何检查裕量，不替代账本的投掷距离。

## 4. 最低持物姿态与短反馈

先做可读的灰盒表现，不必先制作专用投掷动画。

1. 在 **ABP_MothPlayer Event Graph** 的角色 Cast 成功后，读取 **Is Carrying Device** 为动画布尔快照 `IsCarrying`；读取 **Get Action State** 为行动状态快照。角色无效时复位这些值。
2. 原 AO_Rifle 的动画 Alpha 改为 `IsCarrying ? 0 : AimBlendAlpha`。右键依旧控制镜头和投掷方向，只停用持枪 AO 对携带姿态的扭曲。
3. 在现有 **UpperBody Slot 输出 → Layered Blend per Bone.Blend Pose 0** 之间插入 **Blend Poses by Bool**，Active Value 接 IsCarrying。False 接原 UpperBody Slot 输出；True 接一个兼容骨架的非 Additive 持物基础姿态。灰盒可用已用于预览的 `MF_Rifle_Idle_ADS` 普通姿态（先核对该序列非 Additive），用 Sequence Evaluator 固定一帧；保持球在手中可读，后续替换专用 Carry Pose。
4. Layered Blend 的 Base Pose 继续用原移动/腾空基础，沿用 `spine_01` 上半身分层；Bool 混合时间可先设 0.1 秒作为表现过渡。腿部、跳跃和移动继续由原状态机驱动。末端 IK/Control Rig 若把手拉回枪柄，携带时关闭该手臂约束，保留脚部 IK。
5. 在 **BP_ThirdPersonCharacter Event Graph** 添加 **Event On Device Interaction Rejected**：Reason → Text To String → Print String，Duration 约 1 秒。这是当前灰盒的受阻/超距/恢复提示，之后由 T18 HUD 接管。
6. 添加 **Event On Device Released**，根据 bThrown 打印“已投掷”或“已放下”，Duration 约 0.25 秒；结合持物到持枪的短混合验证释放反馈。蓝图不再 Detach、不写速度、不用 Delay 解除行动锁。
7. 有合适非 Root Motion 的上半身释放动画时，可制作 **AM_MothDeviceThrow**，使用已验证的 UpperBody Slot，将它赋给角色 **Throw Montage**。没有该资产时留空；短提示和姿态过渡仍须配置并验收。

## 5. Automation

打开 **Tools → Test Automation**（或 Session Frontend → Automation），搜索 `MothEffect.Devices`。保留 T06 三项测试，并运行新增三项：

- `MothEffect.Devices.ReleaseSafety.ThinWall`：空路径可释放、两端空旷但中间薄墙阻挡、关闭墙碰撞后可释放。
- `MothEffect.Devices.ReleaseSafety.EndpointOccupancy`：物理球实际缩放改变净空结果、终点/起点占用被拒绝，包含零长度查询。
- `MothEffect.Devices.ReleaseSafety.PawnAndIgnoredActors`：即使 Visibility 忽略 Pawn，物理释放仍受阻；显式忽略自己和无碰撞对象才放行。

这些测试使用隔离 World 和原生碰撞组件，不依赖玩家 BP/动画资产；它们不验证 E 映射、真实 Held 状态、松键时序、换弹竞争或动画，以下 PIE 仍需执行。当前六项的本轮结果均为未执行。

## 6. PIE 顺序与记录

测试房放两件 BP_DeviceBase、一个薄墙/掩体和原 T05 伤害靶，打开 **Log Device Events**，保留 T06 状态打印。角色 `On Player Action State Changed` 可临时打印新状态。调试 Print String 只用于 Development，正式公开包用 HUD 提示。

| 测试分支 | 操作与应观察结果 | 当前结果 |
|---|---|---|
| TC02 范围/遮挡/单持有 | 准星对近处球按 E：Dormant → Held，玩家 Carrying，球跟手且枪隐藏；超距/墙后失败。持有时对第二件按 E 只放下当前件；再次 E 才拾取下一件，无复制/丢失。放下回 Dormant/Ready | 未执行 |
| TC06、TC37 按压与恢复 | Held 左键成功投掷后长按超过恢复时间，不扣弹/不续射；松键再按才开火。恢复中提前再按并长按跨过结束，同样必须再松键/新按下。恢复时 R/E 不成功 | 未执行 |
| TC17、TC37 安全释放 | 正对近墙、薄墙、掩体边、其他角色/物理物件，分别 E 放下和左键投掷；受阻仍 Held/Carrying，有提示且无恢复。背贴墙、抬头/低头重测，释放体积不进墙，不从相机位置穿墙。再验步枪枪口遮挡分支 | 未执行 |
| TC36 换弹竞争 | 半匣开始 R，成功 E 拾取只取消未完成任务且不补弹；等到旧完成时刻仍不补弹/不覆盖 Carrying。超距、遮挡、失效候选 E 失败，原换弹按时补满 | 未执行 |
| TC06 持物无护盾 | Held 直接调用 Try Activate 或命中接口返回 false；用独立射手的步枪/有效伤害命中玩家，球不阻挡且玩家扣血。敌人/P01 尚未实现的来源随 T08/T14 补测 | 未执行 |
| TC31、TC38 生命周期/表现 | 持物可移动/跳跃/瞄准/冲刺；停止释放 Montage 不改变恢复。暂停冻结恢复，恢复后仍需实际松键；失焦不续射。Held 死亡/Set Gameplay Enabled(false) 不掉落并清引用；意外 Destroy Device 后存活玩家回 Ready、恢复枪显示 | 未执行 |

人工难以在短恢复时间内按键时，可临时调大 BP 的 Min Gun Recovery Seconds，记录它为测试覆盖值，测完恢复账本值并再验正常时序，不把扩大时间写成玩法调参结论。用于死亡的测试命中须使用新 HitId、非零方向及足够伤害；受伤/持物始终复用已有 HealthComponent，不另写扣血。

保存实际构建标识/commit、日期/执行人、UE 版本、PIE/独立包、参数版本与覆盖值、测试结果、日志/录屏。T07 当前验收和独立 Windows 包尚未执行；完整 TC06 的未来敌弹分支及 G1 空中发射随后续任务补测。

## 7. 拾取失败与“启动即激活”的排查

2026-10-08 用户报告 PIE 启动后似乎已激活。核对当时本机日志：14:59:34 启动的那轮 PIE 在 15:00:27 退出时记录 `Dormant -> Destroyed`，没有 Active 转换；上一轮 14:58:53 的 `Dormant -> Active` 伴随 `source=BP_Rifle_C_0`、`accepted=1`，是一次步枪命中。此记录只说明这两轮，不替代后续重现的实例状态检查。

旧代码把非道具命中/无效候选/交互未开放都显示为“只能拾取尚未启动的道具”，该提示不能证明 Active。本次拆开拒绝原因，并在 Development 的 E 命中日志中记录 `pickup trace actor=... component=... state=... point=...`；尚未编译或重测。

- `state=NotDevice`：射线命中地面、掩体等对象，没有选中道具。靠近后用屏幕中心准星对准球，距离仍按 throw.pickupRangeCm 限制。
- `state=EDeviceState::Dormant`：候选未激活，继续根据超距、遮挡、挂点或交互未开放的具体提示排查。
- `state=EDeviceState::Active`：检查此前的 `Dormant -> Active` 和 `accepted=1` 命中日志，确认 source/instigator。在输入/蓝图排查前，先区分开始时已是 Active 与开始后被命中。

运行状态用 PIE 的实际 BP_DeviceBase 实例 → Get Device State → 枚举转字符串 → Print String 核对。初次核对可以在装置 BeginPlay 仅打印状态，运行中暂停后在实例 Details 查 Device State。打印不得调用 Try Activate；测试输入先不按左键，使用键盘启动 PIE 并用 E 拾取。只有能重现初始 Active 时，再检查装置/关卡蓝图的测试激活调用和保存的实例状态。
