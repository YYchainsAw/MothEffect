# Moth Effect（飞蛾效应）：T15 三外观与固定掉落接入

版本 v0.3 · 2026-10-12 · 状态：原生验证通过；保存关卡实例不掉落已修复，新增NullRHI关卡PIE验证通过；当前Murdock图形PIE待复测验收

Refs #17，DEC34、TC20；核心f8e3216，保存资产与测试53ba114，最终测试夹具6512650。沿用[T14射击AI](Enemy_Setup.md)和[T13生命周期](Lifecycle_Setup.md)，本地develop逐功能提交。T14 PR #42仍Open、用户确认PIE尚未完成；2026-10-12按用户要求创建[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43)（codex/t15-enemy-drops → codex/t14-shooting-ai），当前Open；待T14合并后再改为main。用户要求保持待验收，#17保持Open / In Progress，当前四项PIE不记通过。

## 1. 预告与固定映射

| 保存敌人（/Game/MothEffect/Enemies/） | 背包与掉落外观 | 固定掉落（/Game/MothEffect/Devices/Drops/） |
|---|---|---|
| BP_EnemyLauncher | 青色三角棱柱，D01 / UP | BP_DropLauncher，继承正式BP_DeviceLauncher |
| BP_EnemyBomb | 橙色球，D02 / BLAST | BP_DropBomb，继承正式BP_DeviceBomb |
| BP_EnemyEmitter | 紫色长条，D03 / FIRE | BP_DropEmitter，继承正式BP_DeviceEmitter |

三个敌人都继承BP_EnemyBase，不复制AI；血量60、预警0.5s和射击尝试间隔1.2s不变。背包自动读取固定掉落类的网格与材质，文字另外表示类型，不只依赖颜色。装饰没有碰撞且不影响导航。原T14基础敌人没有配置掉落类，仍不掉落。

三种掉落保留正式父类的物理根、拾取/投掷与激活规则，只改变可见外观。三角棱柱由本项目六个顶点的OBJ生成，球/长条使用Engine BasicShapes；共用本项目Tint材质与三个材质实例。本轮新增12件资产，使用Git LFS，没有修改已有玩家或机关蓝图。

## 2. 死亡、落点与容量

生命死亡先提交幂等标记并保存死亡位置、类型、掉落类和World生命周期世代，再停止AI/碰撞并广播死亡观察者；随后只尝试一次。重复致死、普通Destroy/EndPlay或调试删除不补掉落。死亡/预警/容量观察者重置World管理器或销毁敌人，取消旧世代的生成。

[参数v0.16](../02_Design_Doc/GDD/Gameplay_Parameters.json)和DefaultGame.ini同步：水平搜索150cm、向下探地1500cm、球体净空2cm、支持面法线Z至少0.65。有限近邻候选同时检查支持面、出生球体和死亡点至出生点的真实阻挡，避免墙内生成或穿墙移位；已有机关是障碍，不当作稳定承托面。空中死亡没有附近地面时，在原死亡点净空处生成真实物理落体；不能悬浮、附着或冻结。

出生位置有效后才走统一SpawnDevice入口；24件满额回收最老Dormant，Held/Active受保护，全部受保护则拒绝。容量回调后再次检查位置和世代；被拒绝的死亡不会在之后空出名额时补发。成功对象初态Dormant、启用物理模拟，并正常计入生命周期。

## 3. 当前 PIE 验收清单

打开`/Game/MothEffect/Tests/L_T15Drops`进入PIE：左侧D01平地、中间D02墙边、右侧D03约20度坡地，三个普通射击敌人共用AI；地图保存静态NavMesh，不含脚本击杀。敌人会追近，实际死亡位置随战斗变化；验证墙边/坡地时注意把对应敌人留在该地形附近，必要时退出PIE恢复场景。

2026-10-12用户实测发现保存的三个敌人不掉落。核查蓝图CDO的固定类型/掉落类正确，但关卡实例的`DropDeviceClass=None`且`DropKind=Emitter`；原自动化新生成的敌人没有覆盖此问题。已只替换本脚本拥有的三个实例，保留位置/旋转/标签并保存地图；生成器现在在修复、保存和重载后检查实例。请停止PIE并重新加载`L_T15Drops`，避免继续使用编辑器内存中的旧地图。三类敌人应显示类型背包，死亡各生成对应一件Dormant。

| Development控制台 | 用途 |
|---|---|
| MothSpawnDropEnemy 0 / 1 / 2 | 在玩家前方约9m生成对应D01/D02/D03敌人，碰撞无法调整时拒绝；仅调试 |
| MothSpawnDevice 0 / 1 / 2 1 | 生成一件正式D01/D02/D03，用于真实弹丸或爆炸击杀 |
| MothSpawnDevice 0 24 | 生成容量检查用的普通Dormant（满额时可能按规则回收） |
| MothEnemyStats / MothLifecycleStats | 观察敌人血量/AI与当前机关、Active/P01计数 |
| MothResetEnemies / MothResetDevices | 分别调试删除敌人，或重置道具/P01；不代替T16完整重开 |

1. **辨认与对应**：从不同角度和距离能读出青三角D01、橙球D02、紫长条D03；分别击杀，每次只出现与背包一致的一件掉落。三类都使用追踪、FIRING预警和正式P01。
2. **真实击杀与单次掉落**：用W01正常三次有效命中，再用机关P01和D02爆炸重测；尸体继续受击不重复生成，死亡短反馈结束后道具仍保留。
3. **墙边、坡地与拾取**：在墙旁、坡面及已有机关占位处击杀；掉落不埋墙/地，附近可拾取、放下、投掷和正常激活。空中受击死亡后掉落应自由落下。
4. **容量与清理**：生成24件Dormant后击杀，应回收最老Dormant、总数保持24，手中Held不被替换；全保护24件及回调重置已有原生证据。调试删除活敌人不掉落；暂停/结束PIE/重新进入不留下额外道具。

以上当前角色视觉与操作尚未得到用户通过确认。完整TC20、T16三波/敌人上限/整局重开、T17最终竞技场和独立包/G3继续按实际范围验收。原生夹具隔离AI移动与玩家输入，使用真实武器命中、Chaos、拾取和生命周期，不代替图形PIE或录像。

## 4. 自动化与证据

Editor中运行`Automation RunTests MothEffect.EnemyDrops`：新增11项首轮及最终复测全部Success，导出errors/warnings=0；生产实现全量88项Editor通过，测试夹具修正后2项Game导航也通过。原工程Editor Win64 Development构建成功，最终构建14.98s。覆盖保存类/颜色/形状与尺寸重载、W01/P01/D02对全部三类实际击杀、死亡重入、实际物理落地/拾取/空中落体、满额回收/全Held拒绝、回调重置与出生位置复核。完整范围和失败修正另见[T15报告](../06_Test_Doc/T15_Report_2026-10-12.md)与[验证摘录](../06_Test_Doc/Evidence/T15_Verification_2026-10-12.log)。
