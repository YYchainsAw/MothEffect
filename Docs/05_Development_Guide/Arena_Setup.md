# Moth Effect：T17 主竞技场灰盒接入与验收

版本 v0.1 · 2026-10-12 · [Issue #19](https://github.com/YYchainsAw/MothEffect/issues/19) · 当前图形 PIE 待验收

主场地为 `/Game/MothEffect/Maps/L_Arena_01`，UE 中打开 `Content/MothEffect/Maps/L_Arena_01`。保存后的默认启动关卡指向它；原有验证地图保留各自用途。进入 PIE 时导演处于 Ready，控制台输入 `MothStartEncounter` 开始，`MothFlowStats` 查看阶段/波数/存活/待生成，`MothRestartEncounter` 清理并重开。完整标题、教学和 HUD 属于 T18。

## 布局与接入

- 地面 60×50 米，X±3000 / Y±2500cm；玩家起点 (-2200,-1800,100)，朝 +Y。A 起步、B 连锁、C 压力区只以灰盒地面色和文字区分，无内隔墙；北/南两侧各约4米回转通路。
- 六个90cm低掩体、三个180cm高掩体按规格摆放。C区平台中心 (1800,1200)，顶面120cm、4×4米；西/东缓坡各水平5米、宽3米、坡度约13.50°，顶面精确衔接地面/平台。外围墙300cm高，开放顶部，安全体积仍由导演管理。
- 八个出生点沿用 T16 数据；地图只放一个保存的 BP_WaveDirector，不预放敌人或机关。波次4 / 3+3 / 4+4、种类/等待/额度均保持参数v0.17，掉落沿用已修复的三种敌人子类。
- 地面、掩体、平台、坡道使用 Engine Cube 和原创灰盒材质、BlockAll静态碰撞；可支撑面标记 MothColumnSurface。灯光可移动，无天花板，静态 NavMesh 已构建并保存。
- 布局源见 [T17_Arena_Layout.json](../../SourceArt/Levels/T17_Arena_Layout.json)，资产生成见 [create_t17_arena.py](../../scripts/create_t17_arena.py)。脚本仅能重建其 Saved/Automation/T17/CreatedPackages.json 已认领的八个包，保存后重载核对出生引用和完整路径，不能覆盖未认领包；不保存玩家蓝图或 SaveAll。

## 本轮需要用户验收的四组

1. **场地与相机（TC17）**：从A区走到B/C区，沿南/北回转通路绕一圈，分别从两侧坡道走上/下平台。在外墙背贴、低/高掩体旁、平台边缘上下转动视角，确认相机没有穿墙、角色不会卡在接缝、三区和敌人/机关轮廓可辨认。
2. **射击、投掷与弹射（TC17）**：低掩体允许肩高射击，高掩体能挡弹；枪口贴高掩体时不会越墙命中后方。拾取道具后在外墙/掩体旁、仰视/俯视投掷，确认不塞墙、不穿墙。D01在开阔地面/平台激活能正常起柱和弹射；贴墙/障碍时净空拒绝正确，不把角色挤进几何体。
3. **AI与完整三波（TC21）**：MothStartEncounter，从A区开始，借掩体改变视线并登上平台，观察完整开火预警、遮挡取消及绕障。完成4→3+3→4+4，存活与待生成同为0才清波；三种敌人正常掉落，八点均不长期卡死，无平台上/墙外敌人阻止胜利。可在 Ready 时先用既有 MothSpawnDropEnemy 调试入口检查单敌人。
4. **界外、保留与重开（TC32）**：把有效 Dormant 留到下一波，确认没有刷新闲置寿命；到胜利/失败后伤害、机关、P01和出生停止，MothRestartEncounter恢复A区起点、100HP/24弹、Ready和空计数。玩家越过安全体积判负，敌人界外注销且不因界外掉落，道具/弹丸界外清理；界外场景可沿用既有调试入口或编辑器移动，不能只观察外围墙碰撞就记通过。

这四组仍待用户记录。原生保存路径、真实 CharacterMovement、P01实体碰撞和三波/重开结果分别列在 [T17报告](../06_Test_Doc/T17_Report_2026-10-12.md)，不能代替本清单的真实窗口输入和当前 Murdock 图形表现。

## 复现助手验证

在原工程 UE 5.8 Win64 Development Editor 构建后，用 UnrealEditor-Cmd.exe 加工程路径和以下参数：

```text
/Game/MothEffect/Maps/L_Arena_01 -game -unattended -NullRHI -nosound -NoLiveCoding
-ExecCmds="Automation RunTests MothEffect.ArenaRuntime+MothEffect.WaveFlowRuntime"
-TestExit="Automation Test Queue Empty" -ReportExportPath=<Saved下绝对路径>
```

三项 ArenaRuntime 核对保存 NavMesh、出生/回转路线、相机通道/支持面/柱子净空、实际P01挡高掩体和越低掩体、真实角色分别步行两条坡道；五项 WaveFlowRuntime 复用实际关卡三波、物理占位/预警中断、界外/暂停、延迟出生回调与十次 OpenLevel 重开。NullRHI不提供画面证据。重新生成关卡使用 `-ExecutePythonScript=<create_t17_arena.py绝对路径>`，保持自然 Editor Tick，成功必须出现 `T17_ARENA_VERIFIED`，不能只看退出码。
