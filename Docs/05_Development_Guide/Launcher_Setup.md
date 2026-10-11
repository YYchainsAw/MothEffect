# Moth Effect（飞蛾效应）：T11 探地、升柱与弹射接入

版本 v0.1 · 2026-10-11 · 状态：源码、灰盒、Editor 构建及 11 项新增 Automation 通过；当前 PIE 待验收

交付提交：`a5903f6ad852d13486ccece6b1b41b2dd3353555`，Refs #13。全量 43 项 Automation 已通过，结果与边界见 [T11 报告](../06_Test_Doc/T11_Report_2026-10-11.md) 和 [验证摘录](../06_Test_Doc/Evidence/T11_Verification_2026-10-11.log)。

## 1. 测试房接入

关闭旧编辑器会话，再打开工程，使用现有 BP_MothGameMode / Murdock 测试房。把 `/Game/MothEffect/Devices/BP_DeviceLauncher` 拖入房间；它已经引用 `/Game/MothEffect/Devices/BP_LaunchColumn`，柱子由种子启动后生成，无需手动放柱子。

选择允许起柱的地面，在 Details 的 **Actor → Tags** 添加 `MothColumnSurface`（不是 Gameplay Tags）；也可在具体承托组件的 Component Tags 添加同名标签。承托组件必须是 WorldStatic，Block `JamGroundTrace` 和 WorldDynamic。动态箱子、玩家、其他装置和生成柱子不会成为承托面。墙和天花板须保留对 WorldDynamic 的 Block，才能作为实体净空障碍。

已有 BP_MothPlayer 工作区改动保留给用户。本轮没有修改现有地图、地面标签或角色资产，因此第一次测试前必须设置地面标签。

| 配置位置 | 参数 / 当前值 |
|---|---|
| BP_DeviceLauncher | Kind=Launcher；Column Class=BP_LaunchColumn；向下距离 1500cm；法线 Z 下限 0.65；要求承托标签 |
| BP_LaunchColumn | 高 350cm，方形半宽 70cm；伸出/停留/收回 0.2/1.5/0.3s |
| BP_LaunchColumn | 角色目标 Z=1000cm/s，道具目标 Z=1200cm/s；底部接触容差 1cm |
| BP_DeviceLauncher | Spent 反馈残留 1s；开发日志开启 |
| ExternalImpulseComponent | 最终速度上限 1600cm/s，爆炸合并后再覆盖柱子 Z |

数值权威为 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) v0.13。灰盒种子使用引擎 Sphere，柱子使用 Cube 和 BasicShapeMaterial；保存后已在另一进程重新加载并执行原生激活。灰盒反馈由标签和材质颜色提供，最终视听反馈随 T19 接入。

## 2. 已实现边界

种子在 Active 通知前保存自己的激活位置，随后沿世界 -Z 探测。ImpactPoint、种子旋转和表现回调中的移动不改变落点。首个阻挡静态面必须满足标签与法线条件；失败消耗种子，不重试，显示红色失效提示并触发 `OnLauncherFailed`。

落点 XY 固定，柱子保持世界竖直；斜面按法线计算方形底座最高接触高度。最终整个方形柱体先检查静态净空，只排除底部接触容差，不忽略整个地面 Actor 或组件。平台边缘采用中心探地：中心仍在合格平台上且最终空间净空可起柱；中心越过边缘、无地面或超过范围则失败。该规则不承诺整个底座都在平台范围内，详见 DEC30。

伸出期间，实体 Collider 关闭；RiseQuery 按已经覆盖的上升体积查询角色与可移动根，每个 Actor 只弹射一次，后来进入已覆盖体积的目标仍可弹射。角色使用 CharacterMovement，物理道具使用根组件；统一帧末处理保留 XY、覆盖 Z，再限速，不直接伤害或启动其他机关。Held 不单独运动，Active D03 保留原发射方向。

完全伸出后停止新增弹射，在剩余停留时间检查全部动态阻挡占用和静态空间。净空才启用 WorldDynamic 实体支撑与弹道阻挡；始终占用则本次跳过实体阶段。未就绪为灰色 `RISING/WAIT`，就绪为绿色 `SOLID`；收回显示 `RETRACT`，先关查询/阻挡并释放站在柱顶的角色支撑，再降低网格。提前 FinishActivation/玩法禁用进入安全收回；种子 Destroy/EndPlay 则立即销毁柱子，避免遗留碰撞。

## 3. 当前 PIE 验收清单

1. **地面/空中与方向（TC09）**：地面开枪启动；拾取投掷后在空中启动；转动种子重复测试。柱子应固定生成于激活点下方，不能沿种子局部方向、命中点或后续移动起柱。连续命中只启动一次。
2. **失效/几何边界（TC09）**：取消标签、超出 1500cm 向下范围、动态箱子下方、陡坡、低天花板、侧墙和平台边缘分别测试。失败应显示红色提示、消耗种子；合格平地/斜面正常起柱，柱体不穿墙或天花板。
3. **单次弹射（TC10/TC14）**：玩家与 Dormant 道具进入伸出区域，包括在伸出中途进入；同一根柱子对每个目标最多一次，保留水平运动。持物随玩家整体运动，Held 不独立飞离；Active D03 位置改变但世界发射方向不变。柱子不直接伤害、启动其他机关。
4. **动态占用与支撑（TC33）**：用动态阻挡箱占据柱体区域至停留结束，始终不应出现 `SOLID`；另一次在停留期间移开箱子，净空后才变绿。就绪时落在柱顶可站立，普通弹丸/步枪被柱身挡住；伸出和收回时无实体阻挡，不夹住玩家或道具。
5. **暂停/收回/可操作性（TC33/TC35）**：伸出或停留中暂停再恢复，阶段不跳过；柱顶静止时等待收回，应恢复下落。弹射后可转镜头、瞄准/射击，落地后可操作；贴墙重测，无穿墙或悬空支撑。对比 D01/D02 同时作用时的手感；精确同帧顺序已有原生记录。

Output Log 搜索 `Launcher` 查看 hit/origin/base/column 和失败原因。记录测试构建、资产版本、参数覆盖、日志或录屏。正式敌人/掉落随 T14/T15、两种完整自然连锁随 T12、整局清理/重开随 T16；上述未来分支不包含在当前验收确认中。Issue #13 保持 Open / In Progress，完整 G2 未通过，图形 PIE 和独立 Windows 包尚未执行。
