# Moth Effect（飞蛾效应）：T08 发射器 D03 与弹丸 P01 接入

版本 v0.2 · 2026-10-08 · 状态：Devices 九项 Automation 通过；World 清理修正后复测、P01 三项与 PIE 待执行

对应 [T08 / Issue #10](https://github.com/YYchainsAw/MothEffect/issues/10)。规则依据是 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)，参数依据是 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) v0.10。继续使用当前 `develop`；本轮没有创建分支或 PR。用户已授权按小功能进行本地 Git 提交，助手未编译或运行 UE。

## 1. 当前行为与范围

- D03 仍使用原生 `ADeviceBase`，`Device Kind = Emitter`。只有有效弹道命中可以让 Dormant 进入 Active；Held、Active、Spent 不重新启动。碰撞和投掷本身不会启动发射器。
- 第一次激活保存并归一化命中上下文的世界 `ShotDirection`。提示箭头和后续所有 P01 使用这个方向；箭头跟随机身位置，使用绝对世界旋转。Active 机身继续受重力和物理运动影响。
- 激活后 0.1 秒首发，此后按 0.25 秒的计划时刻发射，只有严格早于 3 秒结束时刻的发射机会有效。正常帧率、无遮挡的基线最多 12 发；卡顿跳过错过的时刻，不补发密集弹串，也不延长寿命。
- 出生点按当前机身位置沿锁定方向取“实际物理球半径 + P01 半径 + 2 cm 净距”。出生路径或终点被静态墙体占用时消耗本次发射机会。动态阻挡通过 P01 的普通命中流程处理。
- P01 使用球体根组件和 Projectile Movement 做 Sweep，速度 1400 cm/s、伤害 10、零重力、3 秒独立寿命。首次阻挡命中先锁定已处理状态、停移动并关碰撞，再调用统一弹道接口，最后销毁；目标拒绝激活同样消耗弹丸，不反弹或穿透。
- P01 只忽略直接生成自己的发射器。激活发射器的玩家仅用于伤害归属，没有免伤；其他外部玩家和装置照常命中。发射器结束或销毁后，已经飞出的 P01 保留自己的飞行与寿命。
- 3 秒到期进入 Spent，停止发射、碰撞和物理，隐藏方向提示；灰盒外观保留 0.25 秒后销毁。禁用玩法、主动结束和 EndPlay 清除发射计时。

新增 `JamProjectile` 对象通道供 P01 使用；步枪继续沿用当前 Visibility。装置的 PhysicsBody 不阻挡 P01，独立 ShotCollider 在 Dormant/Active 时阻挡 P01，Held 时两者均关闭。不要把整个项目的射击查询通道自行替换掉。

实现：[装置基类](../../Source/MothEffect/Private/Devices/DeviceBase.cpp)、[P01 弹丸](../../Source/MothEffect/Private/Combat/RuleProjectile.cpp)。当前没有 D01/D02 效果、T12 三种机关自然连锁、T13 容量/闲置过期及 T16 整局胜负重开清理；T07 未测边界与 T09 完整输入回归继续保留待验收。

## 2. 完整编译与资产目录

1. 退出 PIE，Save All，关闭 UE 编辑器。
2. 用现有 IDE 完整编译 `MothEffectEditor / Development Editor / Win64`，成功后重新打开 `MothEffect.uproject`。本次有新 UCLASS、反射字段和碰撞配置，需要关闭编辑器编译，不能仅依赖 Live Coding。
3. 在 Content Browser 的 `/Game/MothEffect` 下建立或复用 `Combat`、`Devices` 目录。以下 BP 资产由用户在编辑器创建，助手没有生成 `.uasset` 或修改 `.umap`。
4. 重开后可在 **Project Settings → Engine → Collision → Object Channels** 核对存在 `JamProjectile`；代码与配置已经定义它，不要另建同名通道或换通道编号。

## 3. 创建 BP_RuleProjectile

1. 在 `/Game/MothEffect/Combat` 右键 **Blueprint Class → All Classes**，搜索 `RuleProjectile`，选它为父类，命名 **BP_RuleProjectile**。
2. 打开 BP，Components 选 **ProjectileMesh**，Static Mesh 设 **Engine → BasicShapes → Sphere**。看不到 Engine 资源时在 Content Browser 设置打开 **Show Engine Content**。
3. ProjectileMesh 的 Relative Location/Rotation 保持零；Relative Scale 设 **0.08 / 0.08 / 0.08**。引擎这个占位球直径 100 cm，缩放后外观直径 8 cm，与 4 cm 的碰撞球半径相符。
4. ProjectileMesh：**Collision Presets = NoCollision**、**Simulate Physics = false**。Actor Scale 保持 1；不调整 CollisionSphere 的 Transform，也不把弹丸改成物理球。可给外观使用已有明亮材质，先保证弹道可见。
5. Class Defaults 搜索下表字段，保留基线。Projectile Movement 的 **Projectile Gravity Scale = 0**、无反弹和 Sweep 由原生设置；不添加蓝图 Tick、Delay、第二套碰撞伤害或寿命逻辑。
6. Compile、Save。

| BP_RuleProjectile 字段 | v0.10 基线 |
|---|---|
| Collision Radius Cm | 4 |
| Speed Cm Per Sec | 1400 |
| Damage | 10 |
| Lifetime Seconds | 3 |

`On Projectile Impact` 是可选表现事件，可以生成短闪光或打印命中 Actor；游戏规则已在事件之前处理。不要在这里再次伤害目标、激活装置或反弹弹丸。

## 4. 创建 BP_DeviceEmitter 与方向提示

1. 在 `/Game/MothEffect/Devices` 新建 Blueprint Class，父类选 **DeviceBase**，命名 **BP_DeviceEmitter**。使用同一个物理装置 Actor 参与拾取、投掷和激活。
2. Components 选 **DeviceMesh**，设置 **Engine → BasicShapes → Sphere**；Relative Scale 设 **0.4 / 0.4 / 0.4**，外观直径 40 cm。Location/Rotation 为零，Collision Presets 为 **NoCollision**，Simulate Physics 为 **false**。Actor Scale 保持 1。
3. Class Defaults：**Device Kind = Emitter**，**Emitter Projectile Class = BP_RuleProjectile**，开启 **Log Device Events**。未赋 BP 类时原生类仍可发射，但原生类没有指定可见网格；需要这个字段才能看到本节配置的弹丸。
4. 检查以下字段。半径、清距和视觉残留是未实测的灰盒起点；调整后同步参数账本、决策和测试记录。
5. Compile、Save，在当前测试房放入 BP_DeviceEmitter。复制若干实例供每轮测试使用；初始位置让球落到地面上，不把物理球嵌进地板或墙内。

| BP_DeviceEmitter 字段 | v0.10 基线 |
|---|---|
| Body Radius Cm / Shot Radius Cm | 20 / 25 |
| Physics Mass Kg | 2 |
| Emitter Duration Seconds | 3 |
| Emitter First Shot Delay Seconds | 0.1 |
| Emitter Shot Interval Seconds | 0.25 |
| Emitter Muzzle Clearance Cm | 2 |
| Emitter Spent Visual Seconds | 0.25 |

Components 里的 **EmitterDirectionMarker** 是原生橙色箭头：原生已设置 **Absolute Rotation**，Active 时朝锁定世界方向，Dormant/Spent 时隐藏。不要让它继承机身旋转，也不要用 BP Tick 每帧朝玩家转向。

需要更醒目的灰盒提示时，可在 EmitterDirectionMarker 下添加一个 Static Mesh 子组件，使用 **Engine → BasicShapes → Cone**，给亮色材质、NoCollision、Simulate Physics=false。将它缩小并移到球体外，例如 Relative Location X=35 cm，再调整相对旋转，让尖端沿父组件的 **+X**。引擎 Cone 默认尖端沿 **+Z**，必须先旋转再核对；球体前后翻滚时提示仍应保持同一世界方向。子组件由原生状态切换传播显隐，蓝图只配置外观。

已有 T06 的 BP_DeviceBase 默认也是 Emitter。本轮新增效果后，它被击中会自动发射并在 3 秒结束；T06 的 J 手动结束检查应在这 3 秒内执行。本轮 PIE 优先使用明确配置了可见弹丸的 BP_DeviceEmitter。

## 5. 运行 Automation

关闭 PIE，打开 **Tools → Session Frontend → Automation**；选择本地编辑器会话，搜索并勾选下面两组，再点击 **Start Tests / Run**。本机菜单入口已在 T07 由用户截图核对。

`MothEffect.Devices` 应包含 9 项：

| 测试 ID | 覆盖范围 |
|---|---|
| MothEffect.Devices.ActivationIsSingleUse | 原 T06 首次激活、重复拒绝与归一化 |
| MothEffect.Devices.CancelDuringActivationCallback | 原 T06 激活回调中取消 |
| MothEffect.Devices.RejectInvalidAndDisabledHits | 原 T06 无效、禁用、暂停命中 |
| MothEffect.Devices.ReleaseSafety.ThinWall | 原 T07 释放路径薄墙 |
| MothEffect.Devices.ReleaseSafety.EndpointOccupancy | 原 T07 起终点占用和实际体积 |
| MothEffect.Devices.ReleaseSafety.PawnAndIgnoredActors | 原 T07 Pawn 阻挡及明确忽略对象 |
| MothEffect.Devices.Emitter.WorldDirectionAndLifetime | 新增：机身位移/旋转不改方向、重复不重启、正常时序 12 发、模拟暂停和 Spent |
| MothEffect.Devices.Emitter.BlockedBirthAndExpiry | 新增：薄墙消耗出生机会、移墙不补发、结束时刻不发射 |
| MothEffect.Devices.Emitter.CancellationStopsTimers | 新增：禁用与销毁停止后续出生，保留已有 P01 |

`MothEffect.Projectiles` 应包含 3 项：

| 测试 ID | 覆盖范围 |
|---|---|
| MothEffect.Projectiles.FirstImpactIsSingleUse | 新增：命中上下文传递、启动 Dormant、重入及重复回调只处理一次 |
| MothEffect.Projectiles.RejectionAndSourceExclusion | 新增：只忽略生成者、激活玩家无豁免、目标拒绝仍消耗、独立寿命 |
| MothEffect.Projectiles.SweptThinWall | 新增：配置半径/缩放在构造前后保持一致、零重力、速度基线、单次 Sweep 不穿薄墙 |

合计 **12 项**。2026-10-08 用户 Test Run 3 已运行 `MothEffect.Devices` 九项并全部 Success，含本轮新增 Emitter 三项和原六项重测；`MothEffect.Projectiles` 三项尚未出现在本轮结果中。原始输出见 [T08 验证记录](../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。Emitter 测试在隔离 World 中手动推进计时，不运行真实物理或弹丸飞行；暂停测试也只证明隔离时钟/计时边界，真实 PIE 暂停仍须验收。Projectile 的薄墙测试执行一次原生移动组件 Sweep，不替代整局物理、玩家输入或视觉验证。

该轮三个 Emitter 案例带 `CleanupWorld ... missing call to EndPlay` 警告：测试只结束各 Actor，没有清除 World 的 BegunPlay 标志。后续提交 `23eb829` 已改为 `UWorld::EndPlay` 后销毁 World，配对 Actor/Subsystem/World 收尾；助手未编译或运行修正。用户重新编译后运行 `MothEffect.Devices.Emitter` 三项，确认仍 Success 且警告不再出现，再运行 `MothEffect.Projectiles` 三项。该轮历史 Success 不表示清理修正已复测通过。

若搜索不到新增项目，核对完整编译成功且编辑器已重开；不要仅刷新旧会话后把缺失项当作通过。保存完整 Test Run 输出和日期，发生失败时保留断言、文件行号及构建日志。

## 6. PIE 具体验收

测试房准备多件 BP_DeviceEmitter、一面可移动的薄墙/掩体、已有伤害靶，打开 Output Log，过滤 `LogMothEffect`。Device 的命中日志包含方向与状态，Emitter 日志包含发射机会、成功出生数、世界方向及当前出生位置。箭头和可见弹丸共同用于核对方向；不能只根据“球动了”判定成功。

| 用例 | 操作与应观察结果 | 当前结果 |
|---|---|---|
| TC03 地面方向 | 分别站到四个方向，用步枪首次击中一件静止装置，每次用新实例。箭头与来弹的世界飞行方向同向，0.1 秒后弹丸沿箭头持续发射。激活后移动、转镜头，方向保持锁定 | 未执行 |
| TC04 空中方向 | E 拾取，左键投掷后**实际松开左键**，等待至少原恢复窗口 0.12 秒，再重新按下射击。分别在上升、顶点附近、下降阶段命中新实例；故意改变投掷与命中方向。装置继续下落/翻滚，箭头方向和 P01 轨迹取第一次命中方向 | 未执行 |
| TC05 操作成功率 | 在安全房熟悉约 2 分钟，再连续记录 10 次“拾取 → 投掷 → 空中射击激活”。逐次记成功/失败和原因，目标至少 8 次成功；保留录屏与计数 | 未执行 |
| TC12 重复与寿命 | 激活后再次射击、尝试 E，不能重启、改向或拾取；从首次命中计约 3 秒进入 Spent，方向提示消失，机身停止，约 0.25 秒后销毁。已飞出的 P01 继续飞行，空旷处各自 3 秒清理 | 未执行 |
| D03 出生净空补充；TC07/TC15 飞行阻挡 | 让激活方向朝贴近球体的静态墙，出生路径/终点受阻时日志记录机会被消耗，墙后不生成 P01。移走墙后只能等后续计划机会；再让飞行 P01 撞薄墙，无穿透/反弹。TC17 的玩家近墙释放/枪口遮挡仍需另行补测 | 未执行 |
| TC07/TC15 外来弹道 | 摆两件装置，让一台 D03 的弹道命中另一 Dormant，后者按来弹方向激活；让 P01 命中独立玩家/伤害靶，角色扣血。原激活玩家走入弹道也会受伤，发射器自身不会被自己的 P01 再触发 | 未执行；敌人来源待 T14 接入 |
| TC06/TC31 适用边界 | Held 不启动、不挡玩家伤害；真实暂停时发射、寿命、物理和 P01 全部冻结，恢复后继续剩余时间。禁用单台发射器停止后续出生，已飞弹不随生成者消失。T09 再补齐长按、恢复期提前按压、失焦与中断 | 未执行 |

投掷是按下就立即释放，当前没有长按蓄力。TC04 的关键是投掷后松键，再按下发出新的步枪请求。用户此前只确认 T07 基础拾取、放下、投掷通过，长按/提前按下、近墙受阻、Held 无护盾和其他边界仍未验收；不能从 TC04 单次成功推定它们全部通过。

TC05 可按以下格式记录，不把未做的尝试预填为成功：

| 尝试 | 成功激活 | 失败原因 / 视频时间点 |
|---|---|---|
| 1–10（实际记录时每次独立一行） | 待测 | 待记录 |

记录构建 SHA、日期/执行人、UE 版本、PIE 环境及参数 v0.10；附 TC03/TC04 的方向录屏、TC05 实际成功数、Automation 完整输出和未通过项。本轮模板见 [T08 报告](../06_Test_Doc/T08_Report_2026-10-08.md)。只有实际完成的用例才能改为通过；静态检查与 Git 提交不作为 G1 验收证据。
