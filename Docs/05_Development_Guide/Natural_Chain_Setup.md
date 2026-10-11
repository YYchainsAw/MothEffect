# Moth Effect（飞蛾效应）：T12 统一弹丸与自然连锁验收

版本 v0.1 · 2026-10-11 · 状态：Editor 构建、全量 53 项 Automation 与保存场景检查通过；当前图形 PIE 待验收

实现提交 `bc3ce4628116fd42dc77a88ee2c8f84b2bfeb246`，Refs #14。参数仍为 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) v0.13，没有为测试修改伤害、推力、发射间隔或寿命。结果与范围见 [T12 报告](../06_Test_Doc/T12_Report_2026-10-11.md)。

## 1. 打开测试房

关闭旧编辑器会话，重新打开工程，再打开 `/Game/MothEffect/Tests/L_T12Chains`。地图引用现有 BP_MothGameMode / BP_MothPlayer；重新 PIE 会恢复初始布局。没有自动启动、关卡连线或目标传送，后续激活全部依赖普通 P01 碰撞。

测试房由引擎 Cube、灯光、文字和 14 件已交付装置构成，五组布局沿世界 Y 每隔 1600cm 排列；地面已配置 BlockAll / WorldStatic 和 MothColumnSurface 标签，原有 ThirdPerson 测试房与玩家资产没有改动。地面发射站前方走廊比试验台低 120cm；高处发射站有阶梯和射击台，用于将步枪轨迹对齐发射高度。

`START_D03` 是应首先启动的发射器，射击时尽量从其后方沿世界 +X；实际激活方向由枪械弹道决定。输出日志核对 `direction`，明显向上/向下或侧偏时调整站位再试。`TARGET` 由连锁命中启动，不需要玩家直接射击。

| 站名 / Y 中心 | 初始布局与操作 | 预期 |
|---|---|---|
| Direct / 0 | 沿 +X 射击 START_D03，观察前方 TARGET_D02 | D03 生成的 P01 击中 D02，爆炸一次；爆炸可推动原发射器，发射方向继续固定 |
| BombHit / 1600 | 先沿 +X 启动 D03，再射击 Y+500 的 START_D02；TARGET_D03 初始位于 Y+280 | 爆炸先推动 Dormant D03，移入原火线后才被 P01 启动 |
| BombMiss / 3200 | 同样操作，目标初始位于 Y+600 | 爆炸把目标推离火线，目标仍 Dormant；与上一站形成位置对照 |
| ColumnHit / 4800 | 上射击台，沿 +X 启动 Z=400 的 D03，再启动地面的 START_D01；目标位于柱体区域 | 普通升柱将 Dormant D03 送入高处火线，实际 P01 命中后启动 |
| ColumnMiss / 6400 | 同样操作，目标横移至 Y+300 | 柱子正常生成，但目标不被送入高处火线，保持 Dormant |

两个移动连锁存在正常的时序窗口：D03 按基线每 0.25s 发射一次，移动目标只短暂穿过火线。漏过时重新 PIE 或调整两个起点的启动间隔；BombHit 可先试在启动 D03 后约 0.25s 的整数倍附近启动 D02。原生测试同时启动起点并控制初始布局，不替代玩家操作成功率验收。

## 2. 当前 PIE 清单

1. 完成 Direct 的普通弹丸连锁，以及 BombHit / ColumnHit 至少一种移动连锁；记录起点、方向、目标和一次激活结果。另一次移动连锁也验证，用于核对两种外力路径。
2. 与 BombMiss / ColumnMiss 对照：保持操作和方向，改变目标摆放后结果随真实运动改变。单独让 D02 爆炸或 D01 起柱，目标没有遭弹丸命中时应一直 Dormant。
3. 对已经 Active 的目标继续观察多发命中：P01 消耗，目标不重复启动、不重置方向/寿命。Direct 可将目标替换成 BP_DeviceEmitter 或 BP_DeviceLauncher，复测另两种目标。
4. 在火线前加入薄墙：墙先挡住 P01，墙后目标不启动；移走墙后再测。玩家进入外部弹道仍受伤，发射者只忽略自己生成的弹丸。
5. 暂停/恢复、移走或销毁发射器后观察已经飞出的 P01：暂停冻结，恢复继续；弹丸按自己的命中或 3s 寿命结束。装置和柱子按自己的阶段结束，重开当前 PIE 无上一轮遗留。

本次未接入正式敌人 AI/开火与掉落（T14/T15），当前同一 P01 路径用 Pawn 生成者和真实 Health 目标做原生验证。容量策略 T13、完整整局重开 T16、独立 Windows 包、帧率/长时间扫测和完整 G2 仍待对应任务；本清单确认不自动通过这些分支。

## 3. 留下可核对的日志

地图上装置的 `Log Device Events` 已开启，Details 可按实例切换；D03 将开关传给生成的 P01。Output Log 搜索 `LogMothEffect` / `Projectile`：

- `event=spawn`：记录 HitId、生成者、出生位置、世界方向、速度和寿命。
- `event=impact`：记录同一 HitId、实际目标、命中点与 `accepted`；Dormant 接受后，其 Device 日志应显示相同 HitId，Active 再命中通常为 accepted=0，但 P01 仍消耗。
- `event=expired`：未命中的 P01 到期清理；生成者销毁后可能显示 source=None，可用出生时 HitId 关联。

记录地图、源码版本、参数覆盖、操作时序、日志或录屏。当前新日志为可选 Development 证据，Shipping 不输出；ABP_MothMurdock 除零及旧 Rig/输入映射警告的历史记录不在本任务中宣称已修复。
