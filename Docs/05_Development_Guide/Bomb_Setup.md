# Moth Effect（飞蛾效应）：T10 爆炸装置接入与验收

版本 v0.1 · 2026-10-11 · 状态：构建、九项新增测试及灰盒资产验证通过，图形 PIE 待验收

源码与参数提交：`5692d76f84e815b3cfb6a6070dd73455bb464bed`，Refs #12。当前 `develop` 本地 Editor 构建与全量 32 项 Automation 已通过；结果见 [T10 报告](../06_Test_Doc/T10_Report_2026-10-11.md) 和 [验证摘录](../06_Test_Doc/Evidence/T10_Verification_2026-10-11.log)。

## 1. 已交付资产与入口

已创建 `/Game/MothEffect/Devices/BP_DeviceBomb`，父类是原生 DeviceBase。资产由 UE Python commandlet 生成、编译、保存，再由另一进程的 Automation 从磁盘加载并执行原生激活。Python 插件只在该次命令中启用，工程插件配置未改。

DeviceMesh 使用引擎 `/Engine/BasicShapes/Sphere`，相对缩放 0.4，对应约 40cm 可见直径；物理球 20cm、射击球 25cm，网格无独立碰撞。无需重做蓝图：关闭旧 UE 会话后重新打开工程，在现有测试房拖入该资产即可。已有 BP_MothPlayer 的用户工作区改动未纳入本轮提交。

| Class Defaults 字段 | 当前值 / 来源 |
|---|---|
| Device Kind | Bomb |
| Bomb Radius Cm / Bomb Damage | 400cm / 60HP |
| Bomb Character Horizontal Velocity Add / Up Velocity Add | 650 / 450cm/s |
| Bomb Device Radial Velocity Add | 1100cm/s |
| ExternalImpulseComponent → Max External Velocity Cm Per Sec | 1600cm/s |
| Bomb Spent Visual Seconds | 0.25s |
| Log Device Events | 开启；仅非 Shipping 记录 |

数值权威为 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) v0.12；本轮仅新增 `bomb.spentVisualSeconds`，既有爆炸和限速数值未改。灰盒不含最终爆炸特效；T19 再接视听反馈。

## 2. 原生结算规则

有效弹道命中先保存装置世界位置作为爆心，再提交 Active。ImpactPoint 和 Active 表现回调中的移动均不改变爆心；重复或重入命中被拒绝。球查询按 Actor 去重，以物理根/胶囊中心为代表点，代表点须在 400cm 内。先冻结全部目标点与遮挡结果，再执行伤害回调，避免某个回调移动其他对象而改变本次范围。

遮挡查询只接受 WorldStatic 且对目标物理根的对象通道返回 Block 的实体；角色、其他机关、WorldDynamic 阻挡物和仅 Overlap 的触发区不作为静态掩体。伤害与推力共用同一可达目标清单。角色通过 HealthComponent 受伤，不忽略引爆者或按阵营过滤；道具只有推力，绝不调用 BallisticReactive。Held 只随持有者运动，持有者仍受伤和推力；Dormant 与 Active D03 可移动，Active D01 不接收外力。

角色水平方向取爆心到胶囊中心的水平径向，再加向上增量；水平距离为零时用激活弹道的水平投影，仍为零则用世界前向。道具沿完整三维径向获得速度增量。随后立即进入 Spent，关闭碰撞并按 0.25s 清理；`OnBombExploded(Center, RadiusCm)` 仅供表现，不应再次造成伤害、施加推力或激活装置。

统一 `IImpulseReceiver / ExternalImpulseComponent` 在当前 World 的 actor tick 结束后结算：原速度 + 全部爆炸增量 → 有柱子事件时替换 Z → 最后限速。事件 ID 去重；角色只提交一次 LaunchCharacter，物理根只提交一次 AddImpulse 的速度变化。暂停保留待处理事件，死亡/结算禁用清除累积事件和已提交但尚未消费的角色 Launch；EndPlay 移除 World 委托。[Epic：LaunchCharacter](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ACharacter/LaunchCharacter) 的速度由下一次移动 Tick 消费。

T14 的正式敌人需接入 HealthComponent、IImpulseReceiver 和 ExternalImpulseComponent，才能使用相同伤害/推力路径。当前原生测试用两个真实玩家角色验证同规则与引爆者友伤，未伪称已经验证正式敌人 AI。

## 3. 当前 PIE 验收步骤

打开现有 BP_MothGameMode 测试房，放两颗 BP_DeviceBomb、一颗 BP_DeviceEmitter 和一块静态墙。使用当前 Murdock 玩家；每次死亡重启 PIE，记录参数覆盖、源码/资产版本与日志或录屏。Output Log 搜索 `Bomb`，查看 HitId、center、radius 和 resolvedTargets。

1. **地面与空中位置 / 单次激活（TC11）**：地面开枪启动，再 E 拾取、左键投掷后用新按压空中启动。爆心应在启动位置；连续命中只爆一次，Spent 不拾取/重启，约 0.25s 后消失。
2. **范围、友伤、持物（TC11）**：玩家胶囊中心放在半径内时，100HP 应降至 40HP；中心在半径外不受伤，即使附加组件伸入范围。角色应向外、向上腾空。持有另一装置时仍受伤并整体运动，Held 不独立飞离；第二次 60HP 伤害应进入既有死亡清理。
3. **静态遮挡（TC11/TC14）**：墙使用 WorldStatic，Block Pawn 和 PhysicsBody。比较墙前/墙后角色与可移动装置；墙后既不扣血也无爆炸推力。补测贴墙、薄墙和爆心贴近掩体，角色/道具不穿墙；Overlap 触发区不能提供免伤。
4. **不直接启动 / 方向保持（TC08/TC13）**：把 Dormant 装置和 Active D03 放入范围。爆炸只能推开前者，不能启动；D03 位置可变但继续沿原世界方向发射。被推出后真正遭普通弹丸命中才启动。新生成的连锁由 T12 完整验收。
5. **叠加与可操作性（TC14/TC35）**：验证 D02 叠加、空中瞄准/射击、落地后可操作及暂停恢复。正式 D01 与爆炸同帧组合随 T11/T12 接入后补测；正式敌我及死亡掉落随 T14/T15 补测。

当前 Issue #12 保持 Open / In Progress，验收勾选等待上述适用 PIE 证据；完整 G2 未通过。原生 Automation 不替代图形表现、完整敌人、真实连锁或独立 Windows 包验收。
