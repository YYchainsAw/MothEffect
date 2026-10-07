# Moth Effect：2026-10-07 进度核查

版本 v0.1 · 2026-10-07 · 香港时间（UTC+8）

本次核对 GitHub main/develop 的 `d589765`、本机 develop 的 `7f009c9`、27 条 Issue 与 Project 状态，并静态检查本机资产和现有编辑器日志。未执行 UE 编译、运行或打包。这里将“源码/资产已有”“任务验收完成”和“可复用模板”分别记录。

本文件保留当次审计的历史结果。同日后续 T05 已完成、PR #34 已合并，T06 原生基础已推进；当前状态见 [开发计划](Development_Plan.md)，机关接入与待测试步骤见 [Device_Setup](Device_Setup.md)。

## 1. 已有实现与待完成分支

| 任务 | 已有的部分 | 还缺什么 |
|---|---|---|
| T01 [#4](https://github.com/YYchainsAw/MothEffect/issues/4) | 10/17 DDL、单人/UE 5.8/无 GAS 方案；本轮确认每天 4–6 小时和少量原创机关/场景资产/UI | 开发机、最低目标机、工具链与平台前置条件的完整实填记录 |
| T02 [#3](https://github.com/YYchainsAw/MothEffect/issues/3) | 项目存在；本机 10/3 日志记录 UE 5.8.0-55116800 与 PIE 启动 | 干净编译成功记录、关闭编辑器后独立 Windows 包启动证据；PIE 不能替代这些验收 |
| T03 [#5](https://github.com/YYchainsAw/MothEffect/issues/5) | Git、忽略/LFS 范围已配置；本轮核对 9 个自有资产与远程指针的内容 SHA 一致 | 最近备份实际恢复一次；记录本机 ThirdPerson/Characters/Input 依赖及其修改后的恢复版本 |
| T04 [#6](https://github.com/YYchainsAw/MothEffect/issues/6) | 越肩相机、镜头 Yaw、瞄准/冲刺互斥与输入绑定有 C++；ABP_MothPlayer/BS_MothRifleLocomotion 存在，资产引用 Rifle/AO/Jump；原验收项均已由 Issue 勾选 | 编译/运行版本、TC34/TC35 基础证据与后续外力腾空分支；Issue 已关闭但 Project 仍待验收 |
| T05 [#7](https://github.com/YYchainsAw/MothEffect/issues/7) | RifleComponent/BP_Rifle、双段/枪口安全检测、射速/弹匣、手动 R 换弹、生命/命中去重/一次死亡与 Montage 入口均有 C++；Issue 前三项已勾选 | 上半身 Slot/骨骼混合及 Fire/Reload Montage 资产与引用核验；TC01/TC24/TC36/TC38 适用分支的实际记录 |
| T06 [#8](https://github.com/YYchainsAw/MothEffect/issues/8) | FHitContext 已含 HitId、SourceActor、InstigatorPawn、ImpactPoint、ShotDirection、Damage；Rifle 已发出 IBallisticReactive 命中调用，Health 使用命中 ID 去重 | 本作机关基类、Dormant/Held/Active/Spent/Destroyed 状态、幂等激活与接入；已有上下文只是前置，不等于“机关接入”验收项完成 |
| T09 [#11](https://github.com/YYchainsAw/MothEffect/issues/11) | 射击取消/实际松键门控、Controller Flush/SetPause 清理、换弹任务 ID 与死亡/EndPlay 清理已有 | Carrying/ThrowRecovery 与同按压只投掷一次、暂停恢复、拾取竞争、结算/重开分支及对应实测 |
| T12 [#14](https://github.com/YYchainsAw/MothEffect/issues/14) | 已有共用命中契约供 P01 复用 | P01 实际弹丸、生成者过滤/寿命/消耗规则与两种自然连锁；目前未见完整实现 |
| T18 [#20](https://github.com/YYchainsAw/MothEffect/issues/20) | Health 的 OnHealthChanged、Rifle 的 OnAmmoChanged/GetReloadProgress 等可供 UI 接入 | 本作 UMG HUD/教程/菜单与真实数据绑定、波次/结果数据来源；事件存在不等于界面完成 |
| T19 [#21](https://github.com/YYchainsAw/MothEffect/issues/21) | OnShotFired/OnEmptyMagazine/OnMuzzleBlocked 等武器事件可复用 | 实际枪口/命中声音与特效、机关启动/方向/状态/过期/危险提示 |
| T20 [#22](https://github.com/YYchainsAw/MothEffect/issues/22) | Rifle 移动/Jump/AO 动画已有引用，BP_Rifle 和本作动画图已存在 | 新确认的三机关外壳、掩体模块、符文/灯光装饰、统一材质、来源与依赖恢复记录及替换后验收 |

可直接继续的顺序：T05 补动画/记录 → T06 复用命中上下文做机关状态 → T07 拾取/投掷 → T08 D03/P01 → T09 完整输入边界。UI 布局与原创资产按 [修订排期](Development_Plan.md) 的独立时段制作，不重新做已经存在的 Rifle/Health 基础。

## 2. 27 条任务的管理状态与判断

源码审计不自动修改用户已勾选的验收项，也不代替 UE 测试。原 T01–T04 关闭状态保留；没有新增关闭任务。T06/T09/T12 的前置和 T18/T19/T20 的可复用材料以文字标注，Project Status 仍反映实际管理状态。

| ID / Issue | Issue / Project | 审计判断 |
|---|---|---|
| T01 [#4](https://github.com/YYchainsAw/MothEffect/issues/4) | Closed / Done | 范围已定；每日工时本轮确认，环境与平台明细待补 |
| T02 [#3](https://github.com/YYchainsAw/MothEffect/issues/3) | Closed / Done | 有工程/编辑器运行线索；干净编译/独立包证据不足 |
| T03 [#5](https://github.com/YYchainsAw/MothEffect/issues/5) | Closed / Done | Git/LFS 已配置；实际备份恢复与模板依赖恢复待证据 |
| T04 [#6](https://github.com/YYchainsAw/MothEffect/issues/6) | Closed / Pending Acceptance | 移动/动画基础已接入且验收项已勾选，证据待补 |
| T05 [#7](https://github.com/YYchainsAw/MothEffect/issues/7) | Open / In Progress | 武器/生命基础已有；上身动画与验收记录待完成 |
| T06 [#8](https://github.com/YYchainsAw/MothEffect/issues/8) | Open / Todo | 共用命中上下文已有，机关状态与接入未见实现 |
| T07 [#9](https://github.com/YYchainsAw/MothEffect/issues/9) | Open / Todo | 未见本作拾取/放下/安全投掷逻辑与资产 |
| T08 [#10](https://github.com/YYchainsAw/MothEffect/issues/10) | Open / Todo | 未见 D03/P01 与空中启动实现 |
| T09 [#11](https://github.com/YYchainsAw/MothEffect/issues/11) | Open / Todo | 射击中断前置已有，投掷/恢复边界待做 |
| T10 [#12](https://github.com/YYchainsAw/MothEffect/issues/12) | Open / Todo | 未见本作 D02 爆炸/去重/遮挡/推力实现 |
| T11 [#13](https://github.com/YYchainsAw/MothEffect/issues/13) | Open / Todo | 未见本作 D01 探地/升柱/净空/单次弹射实现 |
| T12 [#14](https://github.com/YYchainsAw/MothEffect/issues/14) | Open / Todo | 命中契约可复用，P01/真实连锁未实现 |
| T13 [#15](https://github.com/YYchainsAw/MothEffect/issues/15) | Open / Todo | 未见机关总容量/Active 上限/过期/重开清理实现 |
| T14 [#16](https://github.com/YYchainsAw/MothEffect/issues/16) | Open / Todo | 有模板 Combat AI；未接入本作射击预警/P01/三类掉落 |
| T15 [#17](https://github.com/YYchainsAw/MothEffect/issues/17) | Open / Todo | 未见三种本作敌人标记与确定掉落实现 |
| T16 [#18](https://github.com/YYchainsAw/MothEffect/issues/18) | Open / Todo | GameMode C++ 仍为 stub，未见本作三波/胜负/重开实现 |
| T17 [#19](https://github.com/YYchainsAw/MothEffect/issues/19) | Open / Todo | 仍用 ThirdPerson 默认地图；Content/MothEffect 无本作地图 |
| T18 [#20](https://github.com/YYchainsAw/MothEffect/issues/20) | Open / Todo | 血量/弹药接口可复用，未见本作 HUD/菜单；预留 4–6 小时 |
| T19 [#21](https://github.com/YYchainsAw/MothEffect/issues/21) | Open / Todo | 武器事件已有，实际反馈未验收；预留 2–3 小时 |
| T20 [#22](https://github.com/YYchainsAw/MothEffect/issues/22) | Open / Todo | 动画/步枪已有，原创资产与整合待做；预留 6–8 小时 |
| T21 [#23](https://github.com/YYchainsAw/MothEffect/issues/23) | Open / Todo | 三波闭环未实现，独立包内部验收未执行 |
| T22 [#24](https://github.com/YYchainsAw/MothEffect/issues/24) | Open / Todo | 未见一人外部盲玩观察记录 |
| T23 [#25](https://github.com/YYchainsAw/MothEffect/issues/25) | Open / Todo | 已有局部移动代码调参；本任务所需盲玩问题/回归未完成 |
| T24 [#26](https://github.com/YYchainsAw/MothEffect/issues/26) | Open / Todo | 未见最大密度性能/稳定性与边界验收记录 |
| T25 [#27](https://github.com/YYchainsAw/MothEffect/issues/27) | Open / Todo | 未见候选压缩包重新解压与阻断回归记录 |
| T26 [#28](https://github.com/YYchainsAw/MothEffect/issues/28) | Open / Todo | 尚无对应最终实机版本的提交物料与资源说明 |
| T27 [#29](https://github.com/YYchainsAw/MothEffect/issues/29) | Open / Todo | 未见最终包上传/平台自测/提审及实际后台状态 |

## 3. 核对证据与边界

- 对比本机与远程：Source/Config 共 99 个文本文件，规范化换行后内容一致；Content/MothEffect 的 9 个二进制资产分别匹配当前 LFS 指针 SHA-256。本机工作树干净，尚未同步最新仓库配置/Docs；核对时并未替换或切换用户的 UE 工程。
- [玩家 Character](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Private/Characters/MothEffectCharacter.cpp#L277) 与 [Rifle](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Private/Weapons/Rifle.cpp#L101) 提供移动/射击/行动状态/Montage 入口；[Health](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Private/Components/HealthComponent.cpp#L27) 处理命中去重、生命和死亡。
- [命中上下文](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Public/Types/HitContext.h) 与 [命中接口](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Public/Interfaces/BallisticReactive.h) 已存在；[Controller](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Private/Framework/MothEffectPlayerController.cpp#L70) 有 FlushPressedKeys/SetPause 保护。
- 本作资产共 9 个：BP_Rifle、BP_ShootingTarget、IMC_MothGameplay、四个输入 Action、ABP_MothPlayer、BS_MothRifleLocomotion。未见自有机关、P01、HUD、关卡或 Fire/Reload Montage 资产。资产存在和二进制中的引用名不能证明图表接线、编译与播放正确。
- 默认入口仍是 [ThirdPerson 地图/游戏模式](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Config/DefaultEngine.ini#L1)，[本作 GameMode](https://github.com/YYchainsAw/MothEffect/blob/d58976514cd57c7b194aef39d3018010d0b7d25d/Source/MothEffect/Private/Framework/MothEffectGameMode.cpp) 仍为 stub。Variants 中的 AI/LifeBar/JumpPad 是模板示例，缺少本作接入证据，不能当作 T11/T14/T16/T18 完成。
- 本机 `Saved/Logs/MothEffect.log` 记录 10/3 的 UE 5.8.0-55116800 与多次 PIE；早期日志有输入/步枪引用缺失警告，最后一轮 PIE 未出现同类警告。它只能说明曾启动编辑器试玩，不能推断 TC 已通过；日志未上传到公开仓库。
- 在工程常用 Builds/Packages/Releases/Dist/Artifacts 目录中未找到包体目录，也未在现有任务记录找到独立包验收证据。外部位置可能另存有包体，不能据此断言从未打包；需要用户把对应版本与结果补入 T02/T21/T25。
- 本机 BP_ThirdPersonCharacter 引用本作动画图、输入和 BP_Rifle，但 ThirdPerson 等资产被当前 Git 范围排除。T03 应保留这些修改后依赖的恢复版本，避免新克隆丢失本机接入成果；不擅自改变用户既定提交范围。

本次不补填未执行的测试结果，不依据静态检查关闭游戏任务。后续证据应带构建/commit、实际环境、日期、测试分支、结果和录屏/日志链接，按 [测试计划](../06_Test_Doc/Test_Plan.md) 与原 Issue 归档。
