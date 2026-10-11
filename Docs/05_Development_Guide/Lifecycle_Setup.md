# Moth Effect（飞蛾效应）：T13 容量、累计过期与清理接入

版本 v0.1 · 2026-10-11 · 状态：同源码隔离 Editor 构建与66项Automation通过；原工程正式链接和当前角色PIE待验收

实现提交 `f9153fdb3631fa4382dd36d97ef2de39558c138d`，Refs #15；依据 R15/R16、DEC32 和 [玩法参数 v0.14](../02_Design_Doc/GDD/Gameplay_Parameters.json)。基线为总道具 24、Active D03 / D01 各 6、P01 共 64、Dormant 累计闲置 30s、末 5s 警告；本次只新增 Dormant 过期后 0.25s Spent 反馈。

## 1. 运行时接入

`UDeviceLifecycleSubsystem` 随每个 Game/PIE World 自动创建；无需放置管理 Actor。`Config/DefaultGame.ini` 是参数账本 limits 的运行时数值副本，修改上限或寿命时同步两处并重测。未来掉落从该 Subsystem 的 `SpawnDevice` 生成，先预占容量；已有地图直接放置的 Blueprint 在 BeginPlay 登记，同样受上限保护。三种已有 BP 自动继承 LifecycleFeedback 文字组件，本任务不改玩家或旧机关资产。

总数包含 Dormant / Held / Active，D01 仍只算种子一份。满额只回收最早生成的 Dormant；没有 Dormant 则拒绝，Held/Active 不被总数策略删除。新的 D03/D01 激活先安全结束最早启动的同类：旧发射器停止 Timer，旧柱子关闭实体/查询并收回，Spent 立即释放名额。P01 在初始化时预占，首次阻挡、到期、销毁和界外均释放；满额消耗当前发射时刻，没有补发或延长。

Dormant 按 World gameplay time 累计，Held、暂停和玩法禁用冻结；正常放下或投掷只继续剩余时间。最后 5s 显示黄色 `EXPIRES Ns`，Held 隐藏；到期先 Spent / 无碰撞 / 移出计数，再显示 `EXPIRED` 并于 0.25s 后销毁。

安全盒可由未来关卡调用 `SetSafetyBounds(Center, HalfExtent)`；未设置时沿用开启的 WorldSettings KillZ / 世界边界，不假设测试房尺寸等于最终竞技场。界外清理涵盖 Held、Active、Dormant 和 P01，D01 派生柱一起清理。`ResetManagedObjects` 删除本 World 的全部登记装置（含 Spent 残留）及弹丸，并拒绝销毁回调/旧世代请求生成遗留对象；仅重置本任务对象，不代表已实现胜负、波次、玩家血量/弹药或完整 T16 整局重开。

## 2. 当前 PIE 清单

当前编辑器加载旧 DLL，先保存改动并关闭，在 IDE 使用 Development Editor / Win64 构建原工程，再打开；隔离验证的 DLL 仅供独立测试，不更新原工程 Binaries。随后可使用 `/Game/MothEffect/Tests/L_T12Chains` 的地面区域。控制台辅助命令仅 Development 有行为：

| 命令 | 用途 |
|---|---|
| `MothLifecycleStats` | 显示当前总道具、Active D03/D01 与飞行 P01 数量/配置上限，同时写 LogMothEffect |
| `MothSpawnDevice 2 24` | 快速生成 24 个 D03，默认一发只生成一件；Kind 0/1/2 分别为 D01/D02/D03，最多单次请求 64 件，仍走正式容量入口 |
| `MothResetDevices` | 清空本 World 的装置和 P01，便于重复测试；地图其它对象与玩家/波次状态不复位 |

1. **累计过期**：清空后生成一件道具，等待约 25s 出现黄色警告；在警告期拾取，持有超过 30s，应保持 Held。放下/投掷后继续剩余几秒，到期变为 EXPIRED 并消失，不能重新获得完整 30s。对暂停/恢复再做一次，暂停期间不推进。
2. **总数回收**：清空后生成 24 件，拾取一件并启动几件，随后再生成。数量不超过 24，只替换最早的 Dormant，手上和工作中的装置保留。原生测试另覆盖没有任何 Dormant 时拒绝生成及提示，单人无需为了该分支伪造 24 个 Held。
3. **同类 Active**：生成七个 D03 后按顺序快速启动，检查最多六个 Active，第一件停止发射；D01 同理，第一根立即关闭实体和弹射、正常收回后清理。原生短持续时长下操作窗口有限，可以先测 D03，再核对 D01 自动化证据。
4. **弹丸与清理**：观察 P01 结束后数量下降；检查已 Spent 的短效果不占道具/Active 名额。飞行 64 发的饱和和恢复分支由原生测试覆盖，不为测试修改游戏射速或寿命。
5. **重复清理**：携带一件、启动发射器/柱子并让 P01 飞行，调用 MothResetDevices，计数应全部归零，持物引用和柱子实体清除；重复调用仍为零，再生成正常登记。重新 PIE 后恢复地图初始布局，不携带上一轮计数。显式安全盒已有原生测试；默认世界 KillZ/边界 fallback 已实现但未单列场景，最终竞技场的安全盒/玩家判负/敌人注销随 T14/T16/T17 接入后补验收。

保留操作、参数覆盖、LogMothEffect 计数、状态/警告日志及构建版本。容量拒绝会广播 `OnCapacityRejected`，Development 提供短屏幕提示与日志；正式 HUD 接入属于 T18。原生结果与失败历史见 [T13 报告](../06_Test_Doc/T13_Report_2026-10-11.md)。完整 TC19 的敌人数量、TC23 整局重开和 TC32 跨波分支不由本清单自动通过。
