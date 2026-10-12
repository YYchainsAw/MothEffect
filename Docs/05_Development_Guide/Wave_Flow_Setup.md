# Moth Effect（飞蛾效应）：三波、胜负与重开接入

版本 v0.1 · 2026-10-12 · 状态：T16 原生/真实 Game 验证通过，当前图形 PIE 待验收

对应 [T16 #18](https://github.com/YYchainsAw/MothEffect/issues/18)。源码与资产已在本地 develop 提交；T14/T15 的图形验收保持待验收。完整证据见 [T16 报告](../06_Test_Doc/T16_Report_2026-10-12.md)。

## 1. 保存场地与入口

打开 `/Game/MothEffect/Tests/L_T16Flow`，点击 Play，控制台执行 `MothStartEncounter`。场地使用正式 `BP_MothGameMode` 和当前 Murdock 玩家；一个 `BP_WaveDirector` 引用 T15 的三个固定掉落敌人。默认先处于 Ready，启动后开始准备阶段。`MothFlowStats` 查看阶段、波次、存活、待生成与准备时间；`MothRestartEncounter` 重新加载当前关卡。

`L_T16Flow` 是保存静态导航的 60×50m 平地验证场地；八个出生点均有到玩家起点的路径。正式竞技场布局、教学/HUD/标题/结果按钮分别交给 T17/T18。旧测试房没有导演时仍使用现有玩法。生成脚本 [create_t16_flow_range.py](../../scripts/create_t16_flow_range.py) 只创建或修复自身拥有的三个新包，不保存用户玩家蓝图。

## 2. 导演接入契约

正式关卡放一个 `BP_WaveDirector`，位置与旋转保持原点默认值。波次和固定掉落排列来自 [关卡规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，时序/安全距离/上限来自 [参数 v0.17](../02_Design_Doc/GDD/Gameplay_Parameters.json)，运行时 Config 为数值副本。可调整八个 SpawnPoints 和显式世界安全盒；先检查实际 NavMesh、支持面与胶囊净空，再生成，不自动换到其他点。

准备阶段已经计入整波待生成；第二批尚未到时间也计入。警戒圆盘和文字标识显示出生点与 D01/D02/D03；玩家过近、实体占位或没有合法导航时保留队列并重试。预警期间每帧复核，条件再次满足后重新完整预警。实际生成早于 AI 的完整射击预警；不会因镜头朝向跳过射击预警。只有存活与待生成均归零才准备下一波；跨波不重置机关生命周期。

T18 在教程完成后调用 `AMothEffectGameMode::StartEncounter`，或直接调用导演 `StartEncounter`。该入口先关闭生成/伤害并清掉教学机关、Held、弹丸与演示敌人，再恢复玩家玩法并准备首波。`SetPlayer` 只在 Ready 绑定同 World 的存活玩家；默认 Tick 自动找到本地玩家。`OnFlowChanged(Phase, Wave, Alive, Pending)` 与 `GetPreparationRemaining` 可供界面读取；禁止用动画或 HUD 事件替代死亡/清波结算。当前存活数是导演登记的敌人，生成上限还核查 World 内所有存活 EnemyBase；正式战斗中原有调试出敌人命令会拒绝额外出生。

死亡/玩家界外进入 Defeat，第三波完成进入 Victory。生命组件的原生死亡提交通知先于 Blueprint-facing 生命/死亡表现，导演先提交终态、取消待生成、关闭 World-local 伤害与生成，再解除敌人/玩家事件、停止玩家输入/换弹/恢复/外力，销毁 Held、机关、柱子、弹丸和敌人残留。直接 Health/Impulse 入口也检查整局开关。World 的延迟生成回调若结束本局，返回后先核对世代，不能访问已清掉的队列或恢复出生。敌人界外行政销毁并幂等注销，不走生命死亡，不掉落；道具与弹丸继续由 T13 安全盒清理。

P0 重开使用当前关卡 OpenLevel，包括暂停结果页：先清理和归零波次，解除暂停，然后换图。新 World 的玩家生命/弹药、导演、计时器、事件与管理器重新初始化；不把清掉道具等同于恢复整局。未来结果按钮调用 GameMode/导演 `RestartEncounter` 即可。

## 3. 当前 PIE 验收清单

1. **三波和分批**：执行 `MothStartEncounter`，依次消灭三波，观察准备阶段、4 / 3+3 / 4+4、固定掉落类型与存活/待生成计数。第二波第一批提前清完，仍须等待第二批；第三波清完后 `MothFlowStats` 显示 Victory，射击/生成停止。当前尚无 T18 结果面板。
2. **出生预警与占位**：重开后站进 S1 警戒圈再开局，清掉其他敌人；保持 wave=1 / alive=0 / pending=1，不跳波。离开后看完整新预警，再从原 S1 出生。用实体挡住其他出生点也应保留待生成。上一波掉落占着出生点时同样会暂缓，可拾走/移开；跨波保留是正常行为。
3. **失败和重开**：同时保留 Held、Active D03、柱子与飞行弹，让玩家死亡；结算后不能继续扣血/发射，持物和效果清掉。执行 `MothRestartEncounter`，生命/弹药/波次回到初始；连续操作并检查旧效果没有带入新局。换弹与 Held 是互斥动作，另测换弹中死亡/重开；暂停结果页也能重开。
4. **暂停/界外/跨波**：准备或预警中暂停，等待后恢复，游戏时钟不提前出怪。把敌人推出安全盒，计数只减一次且不掉落；玩家界外判负；道具/P01 界外释放额度。保留一个合法 Dormant 到下一波，闲置剩余时间继续累计而非刷新。

当前 Game 日志保留既有 Murdock/Paragon 加载、ABP 除零与空粒子 Notify 警告；见报告逐项记录。助手运行的是 NullRHI 原生/真实 Game 进程，没有记录当前角色图形 PIE 通过。最终场地、完整教程 UI、独立包和完整 G3 继续待对应任务验收。
