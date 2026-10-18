# Moth Effect（飞蛾效应）：变更日志

版本 v0.71 · 2026-10-18 · 单人 UE 5.8 C++ 与蓝图混合开发

CHG075（2026-10-18）：推进第一轮TSR、灯光和统一后处理。SoulCity通过单个临时视觉导演应用可逆光照与低优先级调色，固定手动曝光、低Bloom、无运动模糊和色差，参数可配。首组对比阴影偏暗，已校准曝光并恢复补光；编译、两项视觉Editor专项及正式SoulCity三波Game回归通过，54出生/击杀与24峰值保持。入口/平台两个1600×900、sg3/100%的真实D3D12前后原图及133件资产SHA归档；截图写完后的Python退出崩溃在清理重试后仍存在，原始日志如实保留。动态TSR拖影、实际战斗可读性与GPU性能待验收，未重新打包。见[画面接入](Arena_Visual_Setup.md)。

CHG074（2026-10-18）：按用户选择将三波扩为12/18/24、同屏上限24，追加定时批次而非叠在同一出生点；敌人及弹丸按World和实际类复用，敌人保留死亡动画，弹丸命中/到期/界外归池，完整重置血量、AI、掉落、动画、命中ID、来源与运动状态。活弹容量提高到128，闲置池保留上限32/128，重开仍OpenLevel；出生安全检查与世代收尾保留，击杀统计支持同Actor多次生命。新增对象池专项和历史回归适配，UE5.8完整编译、159项Editor及正式SoulCity Game专项通过：实际54次出生/击杀、24峰值、24创建/30复用；10项对象池专项另覆盖旧弹解除来源身份、阻挡出生、有界保留和重入。性能与硬件PIE待验收，未重新打包。原始失败及修正范围见[自动化记录](../06_Test_Doc/Evidence/CombatPooling/Automation_2026-10-18.json)。配置与实现见[波次接入](Wave_Flow_Setup.md)。

CHG073（2026-10-17）：按用户要求将当前工作区生成 Win64 Shipping 包 `MothEffect_Win64_20261017_145857_a5154b13`。BuildCookRun 全流程 498.41 秒成功，Cook 1,332 包、0 errors／11 既有警告；UI 目录补入 AlwaysCook，修复原生软路径 HUD／Logo 的打包收录风险。成品容器逐项包含 37 个关键地图／模型／UI／语音资源；约 1.40 GB ZIP 的 29 件运行文件独立重解压 SHA 一致，实际 Shipping 启动可见主菜单、SoulCity、暂停和设置页面并关闭进程。包体、校验值和原始证据见 [Windows 包记录](../07_Release/Windows_Package_2026-10-17.md)；完整三波、听感、性能与其他电脑仍待试玩，没有上传、发布、提交或推送。

CHG072（2026-10-17）：按用户要求以 Drongo 不同 Mesh 区分固定掉落：Launcher／D01 使用 Scavenger，Bomb／D02 使用原版 Drongo_GDC，Emitter／D03 使用 AlienInvader。三个子类清除旧材质覆写，使用模型自身原配材质与同一 Drongo 骨架、ABP、开火／死亡动画和现有语音；固定掉落、AI、碰撞和波次规则沿用。隔离保存及重载、三类真实移动／完整预警开火／死亡动画／各一件对应 Dormant 掉落通过，D3D12 外观预览已检查；验证脚本重入与测试位置重复占用的问题修正并留档。仅三个敌人蓝图写回，其他 136 件项目／配置文件与 1,913 件 Drongo 原包文件 SHA 保持；无 C++ 改动，无需重新编译。对应关系与证据见 [Drongo 接入](Drongo_Enemy_Setup.md)；手动整局、性能和独立包未记为通过，未提交或推送。

CHG071（2026-10-17）：按用户选择接入本地 Murdock／Drongo 原配录音和 Kenney Interface Sounds。角色白名单为 15／11 条独立录音，接成功开火、确认击杀、真实扣血、敌人攻击预警和单次死亡；新增四个 UI SoundWave、显式 26 条 VO Cook 标签及按钮绑定。世界音频管理并发、冷却、暂停与旧会话清理，死亡声音可超过敌人尸体寿命；修正终局重复代次失效和受击回调重置后的旧声音问题。原工程常规 Editor 构建通过，41 项音频／生命周期／波次／反馈回归为 34 成功、7 带现有资源或测试夹具警告成功、0 失败；实际 D3D12 音频运行验证按钮、开火、受伤、预警、死亡、暂停恢复及重置，30 个源 WAV 均有非零 PCM。失败测试与脚本 API／夹具修正保留证据，未将主观听感、整局混音或独立包记为通过。配置及证据见 [角色与界面音效接入](Character_Audio_Setup.md)；未提交或推送。

CHG070（2026-10-17）：按用户英文短句与战斗喊声偏好完成音效选型，核对 Paragon 角色 1085 个独立 SoundWave、SoulCity 49 个 SoundWave 与 7 个 Cue，提取实际 Dialogue 文本和可复用 Cue。外部候选覆盖按钮、科幻枪声、换弹/空膛、碰撞、爆炸、机械与播报，记录作者页、当前费用、许可与试听入口；本地原配 VO 优先。新增 [音效清单](Audio_Shortlist_2026-10-17.md)，同步入口和阅读版。本轮仅静态盘点与网页核对，未试听、导入或接线，未修改声音资产或玩法代码。

CHG069（2026-10-17）：按用户要求为三类道具增加 BeginPlay 起的游戏时间出生保护，期间拒绝弹道/爆炸激活，拾取与显式推力可用，放下/投掷不重置；D03 触发时按三维距离选择范围内最近有效存活 AEnemyBase，锁定指向 Actor 中心的三维方向，无目标沿用入射 XY/竖直 +X 后备。保留统一效果延迟、首发延迟和原工作期。参数账本 v0.22 新增 device.spawnActivationProtectionSeconds 与 emitter.targetRangeCm，DEC45/46、交互规则、技术设计与 TC43–TC45 同步；旧报告和已通过证据保留对应历史范围。本轮隔离 UE 5.8 Editor 完整构建与 142 项 Editor Automation 全部通过（139 无警告、3 项带 Paragon/Designer 警告），5 项新增专项全部通过；证据见基础重构接入末节。当前打开的原工程仍加载旧 DLL，需要关闭编辑器后完整编译并重开；硬件 PIE 与手感待验收，未提交或推送。

CHG068（2026-10-16）：接入用户选定 Niagara Examples Pack，八件项目系统副本通过原反馈 Cue 接实际枪口、碰撞、D01/D02/D03 与成功拾取；保留音效和用户准星/UI。反馈 Profile 新增系统、颜色参数、内部缩放、法线、方向及独立最大寿命；新旧效果共用预算，暂停冻结，结算/重试/World 结束销毁持有组件。原工程 Editor 构建与 19 项反馈/Niagara 专项成功，实际 D3D12 八类模拟、暂停同实例冻结/恢复及真实死亡清理通过，九件资产 SHA 写回，123 件其他项目文件与 669 件原包保持。保存配置及脚本接口首轮失败保留；旧命中测试夹具补本地标记和 PlayerState，未改伤害归属规则。配置、截图与边界见 [Niagara_Setup](Niagara_Setup.md)；手动整局、性能与独立包待验收，未提交或推送。

CHG067（2026-10-16）：按用户要求移除旧 AI 原生布局与绘制，用 Colorosse Slate / Kenney 制作保存的 Designer HUD、菜单及敌人生命条。玩家生命放左下，补齐弹药/换弹、波次、交互和操作指南；敌人生命观察真实 HealthComponent，结合距离、屏幕和遮挡显示，死亡解除绑定。旧 54 个生成控件替换，用户准星子控件保留，准星包 SHA 和 HUD 15 个事件节点/引脚/连线一致；四件 Drongo 敌人蓝图指向新生命条类，玩法 AI 与掉落延续前轮。原工程常规 Editor 构建通过，两件 UI 蓝图零编译错误/警告，三项 UI Editor 专项为 2 成功/1 带警告成功；实际 PIE 伤害、菜单旅行、暂停/恢复、击杀反馈、返回标题复位及五张 D3D12 画面验证通过，首轮构建/脚本夹具失败另名保留。编辑与证据见 [Slate_UI_Setup](Slate_UI_Setup.md)。未将手动输入、三波整局、性能或独立包记为通过；第三方和 DesignerProbe 已有警告保留，未提交或推送。

CHG066（2026-10-16）：按用户选择将三类敌人统一为 Drongo，保留既有敌人类、AI、生命与固定掉落；新增项目版原 ABP，删除演示玩家/连击/出生摆姿依赖，移动判定改读实际速度并防止初始化转身计算除零。父蓝图接原开火 Montage 与完整死亡动画，四件敌人蓝图配置配套模型并隐藏灰盒。隔离 UE 5.8 编译、两次保存实例重载、三类实际移动/UpperBody 射击与 P01/完整死亡/固定掉落专项，以及 D3D12 预览通过，进程退出码均 0。用户关闭文件锁编辑器后按 SHA 写回五件包；118 个其他保护文件与 1,913 个 Drongo 原文件保持。初次旋转断言失败保留，修正后重测；具体证据和未验收范围见 [Drongo 接入](Drongo_Enemy_Setup.md)。未修改 C++、用户 HUD 或供应商资产，未提交/推送。

CHG065（2026-10-16）：用户补入 Niagara Examples Pack 与 Paragon Drongo，并明确暂缓 Minions。隔离 UE 5.8 注册两包、检查 `/Game` 硬软引用并实际加载 23 件关键对象；Drongo 主 ABP / Rigging 子 ABP 内存编译为 UpToDate，原 Character 带旧输入映射警告。注册 59 个 NiagaraSystem，其中 15 个实际加载通过；日志退出码 0、0 errors、17 warning 事件，保留旧 Rig 与预览软引用问题。两包原目录/隔离副本 2,582 件文件和 122 件保护范围内项目文件 SHA 均未变，没有保存或写回原包、修改配置或接入运行时外观。位置与证据见 [导入说明](Asset_Import_Setup.md)，实际特效画面、AI 动画、PIE 和独立包未验证；未提交或推送。

CHG064（2026-10-16）：按用户选型和导入授权，新增 Slate / Kenney 的 3,234 张 UI Texture2D，复制 Wraith 的 1,597 件原包文件，保存原路径及来源许可。通过隔离 UE 5.8 二次磁盘读取、运行时硬引用检查、五件 Wraith 关键资产加载和写回 SHA；首轮缺 XRBase 的失败保留，随后补齐插件并以退出码 0 通过。94 件原有文件写回前后 SHA 一致，之后仅新增 uproject 的 XRBase 依赖；未改 HUD、玩家、敌人 AI 或 C++。其余 Niagara / Minions / Drongo 因本机无包，待用户 Fab 添加后核验。位置与边界见 [导入说明](Asset_Import_Setup.md)，未提交或推送。

CHG063（2026-10-16）：根据当前进度新增 [开发者日志](Developer_Log_2026-10-16.md)，覆盖10/14—10/16道具基础重构、SoulCity场景与正式准星接入；分别记录已实现、已有验证和待验收，保留击杀反馈未确认修复、实际操控/性能/独立包的风险，提出下一期整合与候选包计划。同步文档入口及阅读版章节；仅文档整理，未修改游戏实现、运行UE或查询远程任务状态。

CHG062（2026-10-15）：按用户要求将建筑带入战斗场地并收紧场景视野。新增七栋封闭建筑，连续外围楼群与四方向原生城市远景卡；高台掩体略南移以留出转角净空，其余地形、8出生点、6登高连接和波次规则保持。Stage_04隔离生成，v3证据另存V3，仍只提升地图和七件自有材质。建筑实体、外围遮挡射线、新增街道导航、原场景/三波与D3D12八视角的实际结果见 [报告](../06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。不改C++、用户HUD、玩家、原图或默认入口，手动战斗和性能待验收，未提交/推送。

CHG061（2026-10-15）：按用户要求将SoulCity资产用于战斗场地内部主体。新增独立场内模块，使用原生台阶/楼板/波纹金属墙/柱/护栏，以及完整风扇接管设备和法兰管材替换方箱；保留原材质与设备等比缩放。六条连接及17组掩体位置保持，设备/管材/护栏的153组隐藏代理使用与显示相同的原生网格/变换，功能代理总数202。原生资产/材质/缩放/可见Cube限制、保存导航/碰撞、脚本三波及D3D12五视角的实际结果见 [报告](../06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。Stage_03隔离生成，v2备份；不改C++、玩家、HUD、原图、默认入口或第三方源包，未提交/推送，手动操作和性能待验收。

CHG060（2026-10-15）：按用户要求重构完整 SoulCity 战斗街区。新图84×72m，-240/0/360/720cm四档高度、六坡道和17组掩体；71件新功能碰撞替代原19件，更新地图实例的玩家起点、8出生点、安全体积与导航。增加场内机器/货箱/管线/棚架、周边楼群、齿轮、外桥、连续地基和落日冷暖照明。最终保存重载20路径、21支持射线、双次磁盘加载及脚本三波4/6/8共18实际出生、Victory检查与五视角结果见 [报告](../06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。不改C++/用户UI/原图/默认入口，旧版备份保留，仅提升地图和七件自有材质；手动通关、AI完整追击、机关手感、性能与独立包未验收，未提交或推送。

CHG059（2026-10-15）：用户授权用已导入 SoulCity 搭建关卡。新增独立 `L_Arena_SoulCity`，保留 T17 原功能几何、支持标签、八出生点、导演和原 GameMode/玩家/HUD；305 个静态外观实例中 268 个引用 SoulCity，另有冷暖灯光、天空和远景雾，六件新自有材质及地图共七包。生成/检查在隔离副本进行，只提升本轮新包，原 80 件游戏资产逐件 SHA 校验保留。保存/重载导航、真实静态碰撞、首波脚本 PIE 与 D3D12 三视角结果见 [本轮报告](../06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md)。没有改 C++、默认启动地图或用户蓝图，没有将用户手动战斗、三波通关、帧率和独立包记为通过；未提交或推送。

CHG058（2026-10-14）：按用户已导入贴图并自行制作蓝图的分工，补入敌人真实命中/击杀快照、世界内原生与蓝图通知、HUD 拥有玩家筛选和重置/暂停/页面事件；普通命中与致死共用 `bKilled` 分支。新增独立的原生命中 X 开关，保留受伤边框及已恢复的旧 UI 默认创建（`moth.UI.LegacyEnabled=1`）。新增九项 Automation 测试源码，覆盖实际伤害、去重、归属、世代重入、关闭/暂停与跨世界隔离；详见 [准星接入](Crosshair_Setup.md)。用户选择“先完成 C++ 和说明，我稍后自行编译”，本轮未编译、未运行 UE/Automation、未保存或修改 `.uasset`，不以旧构建/测试记录作为本轮通过证据；未提交、推送或创建 PR。

CHG057（2026-10-14）：收尾提交 0e5370a / 4934627 修复延迟会话关闭立即取消、落地关闭模拟顺序及无碰撞道具的重叠掉落/拾取；新增六项 Foundation 并适配既有专项、保存蓝图与真实主地图时序。隔离 UE 5.8 构建通过；首轮 Foundation 5 成功/1 失败、首轮全量 111 成功/3 失败如实保留，最终 114 Editor 与 12 D3D12/音频主地图 Game 全部通过。Editor errors=0 / warnings=50（DesignerProbe），Game errors=0 / warnings=232（导航/第三方 Rig/旧输入/动画/夹具配置/Notify）。180 件源码/配置与 4934627 一致，用户并行玩家/控制器/UI/Build 改动未纳入。后续确认编辑器已关闭，原工程常规构建56.79秒Succeeded，新DLL六项Foundation复测通过，包含用户当前未提交控制器源码但未纳入本轮提交。见 [验证报告](../06_Test_Doc/Device_Refactor_Report_2026-10-14.md) 与原始证据；硬件 PIE、尺寸/手感调优与独立包待验收，四项 Issue 保持 Open / In Progress，未推送/创建 PR。

CHG056（2026-10-14）：用户开始道具基础重构，使用当前 `refactor` 分支、逐项本地提交。新建 [#48](https://github.com/YYchainsAw/MothEffect/issues/48) 地面固定/尺寸配置、[#49](https://github.com/YYchainsAw/MothEffect/issues/49) D03 水平入射投影、[#50](https://github.com/YYchainsAw/MothEffect/issues/50) 三机关共用延迟及 [#51](https://github.com/YYchainsAw/MothEffect/issues/51) 爆炸激活；负责人 YYchainsAw、既有标签、M5 和 Project / In Progress 均已回读，四项保持 Open。落地实体根停止模拟/NoCollision，独立 ShotCollider 保留查询；显式推力/弹射可再次起飞，落地重新固定。D03 锁世界 XY 投影，竖直命中回退 +X；共用延迟后才起效果，D03 原首发延迟及工作寿命继续从效果起动计。D02 开放无遮挡 Dormant 的延迟连锁，Held/Active 保护及独立伤害/激活 HitId 保留。新增 DEC41–DEC44、参数 v0.19 与 [重构接入](Device_Refactor_Setup.md)，同步现行规则/TDD/计划/入口；本轮 UE 编译、原生执行、当前 PIE 与独立包先记未运行，不把此前测试结果移作新规则验收。未创建 PR、推送或关闭任务，用户玩家资产与 Build/ 不纳入本轮。

本文件按日期追加文档与项目变更，保留每次操作当时的实际范围和验证状态。2026-10-03 的未编译/未提交记录是历史，不覆盖后续远程配置、合并和任务状态。当前文档入口为 v0.58，开发状态见 [开发计划](Development_Plan.md)，修改顺序见 [文档维护流程](Documentation_Workflow.md)。

## 1. 变更记录

| 变更 ID | 日期 | 内容 | 原因 | 影响与验证 |
| --- | --- | --- | --- | --- |
| CHG001 | 2026-10-02 | 建立 v0.1 文档基线 | 把创意转换为可执行设计 | 尚未实现，需 G0 至 G4 验证 |
| CHG002 | 2026-10-02 | 确认 UE 5.8 与混合开发，无现成资源 | 用户补充 | [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 使用混合架构，[关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 采用模板与自建灰盒 |
| CHG003 | 2026-10-02 | 更新 v0.2，确认游戏名称与魔法朋克风格（现行名称：Moth Effect / 飞蛾效应） | 用户确定名称与风格 | 更新标题、表现提案和元数据，战斗数值未改 |
| CHG004 | 2026-10-02 | 记录既有模板工程与 MothEffect 模块，复用现有角色/控制器/游戏模式 | 用户创建工程，本次只读核对 | 更新 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)/[开发任务与排期](Development_Plan.md)，T02 标部分完成；编译与打包未执行 |
| CHG005 | 2026-10-02 | 整理 v0.3 文档：统一英文语义文件名、阶段归档、拆分职责、更新导航与阅读版 | 用户要求按已有文档结构规范管理 | 保留所有规则/任务/测试/决策 ID，参数数值未改；澄清 TC29 本地验收与平台自测的执行顺序，校验文件链接与内容保留 |
| CHG006 | 2026-10-03 | 初始化本地 main、连接 GitHub origin、配置 .gitignore 与 Git LFS、补充公开建议 | 用户要求连接仓库并明确禁止提交推送 | 未暂存/提交/推送；T03 标部分完成，恢复仍待验收；标记个人路径与配置凭证字段，未输出凭证值、未修改游戏实现或引擎配置 |
| CHG007 | 2026-10-03 | 将个人磁盘路径改为相对链接/路径，同步 HTML，处理 AndroidFileServer Token | 用户准备自行提交推送，要求清理公开内容 | JSON 路径基准为其文件目录，玩法参数未改；禁用 AFS 用途和网络连接、Token 显式清空；未暂存/提交/推送 |
| CHG008 | 2026-10-03 | Content 下只将 MothEffect 子目录纳入 Git，其余目录和顶层文件排除 | 用户指定提交范围 | 本机资源未删除，LFS 继续用于保留资产；记录当前 ThirdPerson 等依赖不会随仓库分发的限制；未暂存/提交/推送 |
| CHG009 | 2026-10-03 | 统一全部文档游戏名：英文 Moth Effect，中文飞蛾效应 | 用户纠正正式写法并补充中文名 | 更新仓库 README、Markdown、JSON 名称元数据及 HTML 阅读版为 v0.7；工程/模块/路径/类名与玩法参数未改；未暂存/提交/推送 |
| CHG010 | 2026-10-03 | 新增 [候选机关道具](../02_Design_Doc/GDD/Device_Candidates.md)：牵引核心、诱光灯、折光棱镜、静滞锚、换位匣 | 用户要求保存为待选制作道具 | 记录效果、组合、待验证边界与观测目标，同步策划/规则入口/决策/开发计划/HTML；未新增正式任务、数值或实现，D01–D03 首版范围不变 |
| CHG011 | 2026-10-03 | 确认 v0.9 玩家方案：首版无 GAS，复用本地 Rifle 动画，C++ 行动状态/AnimBP 移动状态/上半身 Montage；补齐职责、转换、输入归属、换弹/拾取、动画中断及后续扩展边界 | 用户明确确认上一轮方案并要求补齐文档 | 更新 REQ09/REQ10、R19–R23、DEC22–DEC27，新增 TC34–TC38；同步资源/配置核对/命名/计划/入口/HTML；新增 aimMoveSpeedCmPerSec 与 airControl，其他数值不变且全部未实测；T04/T05/T09 粗估与总工时同步，玩法任务/测试仍未完成 |
| CHG012 | 2026-10-03 | 开始 T04：写入越肩相机、镜头 Yaw 朝向、瞄准 FOV、冲刺互斥与输入 Flush 清理；新增 [玩家第一步接入](Player_Setup.md) | 用户选择助手改 C++、本人配置 UE 资产和试玩；随后明确不由助手编译 | 4 个源码文件修改，暴露 Aim/Sprint 引用与动画读状态；T04 标进行中，输入/动画资产待用户接入；修改后未编译，所有运行测试仍未执行；同步进度与阅读快照 |
| CHG013 | 2026-10-03 | 补充 ABP 的 IsAiming 数据同步、Direction/GroundSpeed、基础腾空与 AimOffset 混合步骤，同步实际输入/动画目录 | 用户报告完成输入、复制 ABP、八方向 BlendSpace 和 IsAiming 变量，要求继续制作 | 复用现有 C++ getter 与 UAnimInstance；静态核对资产存在及 BlendSpace 命名，不推断内部接线或试玩成功；T04 继续进行中，运行测试仍未执行；助手未编译或修改资产，更新阅读快照 |
| CHG014 | 2026-10-03 | 开始 T05：ARifle 双段射击/弹匣/手动换弹、生命与命中接口、行动状态和 Montage 入口；新增 Weapon_Setup，补记用户移动速度代码调参与 AO -1～1 归一化步骤 | 用户报告本轮测试成功并询问下一步，沿已确认协作方式继续制作 | 用户此前只改代码的移动速度调参保留，JSON 未同步且已标明差异；枪械/生命默认参数沿用账本。T04 局部成功由用户报告，T05 进行中；新代码未编译/运行，资产配置和武器验收待用户完成；同步文档链接/阅读快照 |
| CHG015 | 2026-10-03 | 新增角色 Mesh 下的 RifleComponent，使用完整 BP_Rifle 子 Actor 预览/持枪；移除独立 SpawnActor 与旧偏移字段，保留类型/挂点设置并完善绑定/清理 | 用户反馈原方案无法方便地在角色蓝图预览和调整位置 | 组件 Transform 成为偏移来源，旧偏移需手动迁移；类不变时不重新设置 ChildActorClass；组件负责武器生命周期。复核本地 UE 5.8 API 与源码，未编译/预览/试玩，未修改资产；更新接入文档与阅读版 |
| CHG016 | 2026-10-06 | 建立 M1–M6、27 条 T01–T27 Issue 和首版开发 Project，确认 10/17 DDL | 用户要求用 gh 落地任务管理 | 已核对阶段截止与 Issue/任务对照；T01–T03 为 Done，T04 仍为 Pending Acceptance，T05 为 In Progress；记录不足不自动补作完整验收 |
| CHG017 | 2026-10-06 | 配置四个 Project 视图与轻量 Repository checks | 用户要求补 Views 与 Actions | [PR #30](https://github.com/YYchainsAw/MothEffect/pull/30) 已合入 main/develop，PR 及两分支 CI 通过；静态检查不编译/运行 UE |
| CHG018 | 2026-10-06 | 配置开发任务、Bug 和默认 PR 模板 | 用户要求配置 Issue/PR 模板 | [PR #31](https://github.com/YYchainsAw/MothEffect/pull/31) 已合入 main/develop，GitHub 已识别模板，CI 通过；Refs 关联任务，验收与关闭分别记录 |
| CHG019 | 2026-10-06 | 同步 v0.14：入口、六阶段排期、27 条 Issue 对照、当前状态、GitHub 流程及阅读快照，补充快照生成/离线核对脚本 | 用户要求 Docs 同步 | 保留既有 T/G/TC/DEC/CHG ID 和玩法数值；T04 Closed / Pending Acceptance 与缺失记录如实归档，完整游戏验收状态保持待证据；文档链接、快照源摘要/锚点及静态检查验证见对应 PR |
| CHG020 | 2026-10-07 | 核对本机/远程源码、9 个自有资产、日志与 27 条 Issue；确认每天 4–6 小时；为 UI、少量原创机关/场景资产与必要反馈预留工时；修订剩余阶段目标，保持 10/17 DDL | 用户要求检查已完成部分并在开发计划安排 UI/美术，随后确认时间与制作目标 | 新增进度核查，记录 T06/T09 前置及 UI/反馈可复用接口；T18 4–6 小时、T20 6–8 小时，总/剩余预算与 55 小时日程同步；保留原验收勾选和关闭记录，不新增游戏验收通过；同步资源规格/决策/入口/HTML，未编译、运行、打包或修改玩法代码/参数 |
| CHG021 | 2026-10-07 | 记录 T05 动画/当前边界 PIE 由用户确认通过、PR #34 已合并和 Issue #7 Closed / Done；新增 T06 DeviceBase/状态枚举、统一命中激活、拥有者状态提交、碰撞与结束清理、三项自动化测试及 Device_Setup；同步任务/测试/入口/阅读快照，记录当前直接在 develop 开发 | 用户确认 T05 完成后要求继续完成 T06，并明确禁止助手自行增加分支 | 复用 FHitContext/IBallisticReactive，先 Active 后回调/效果，重复命中不覆盖首次证据；参数未改；T06 #8 保持 Open / In Progress。助手核对本机 UE 5.8 源码并执行静态检查，未编译/运行 UE、修改二进制资产或提交/推送；用户完整编译、Automation 和 PIE 待执行，Held 实际操作及效果/容量随后续任务补测 |
| CHG022 | 2026-10-07 | 记录用户 T06 Automation Run 3 两项通过、一项失败；修正 DeviceActivationTests 的 World Context、InitializeActorsForPlay 与 EndPlay 清理，增加初始化检查和首次接口失败时停止后续断言；同步接入/测试/进度记录与阅读快照 | 用户提供首次接口激活失败及 World has no context 日志；本机 UE 5.8 Actor::ProcessEvent 与 UHT 代码核对确认测试环境缺失初始化 | 保留 Execute 接口验证，玩法状态与激活源码未改；用户已提交基础版本 3b0e93f，本轮修正待用户重新编译、运行三项 Automation 与 PIE；助手仅静态核对，不新增通过结果，不提交/推送或创建分支 |
| CHG023 | 2026-10-07 | 将 DeviceActivationTests 中误写的 Engine/URL.h 修正为 Engine/EngineBaseTypes.h，更新接入记录与阅读快照 | 用户报告 MSB3073 / code 6；本机 UBT 日志确认 C1083，UE 5.8 的 FURL 定义实际位于 EngineBaseTypes.h | 核对测试文件 9 个引号 include 均存在，静态差异检查通过；用户已提交前次修正 8085801，本次一行头文件修正待用户重新编译和 Automation 复测；助手未执行 UE 编译 |
| CHG024 | 2026-10-08 | 归档 T06 构建检查成功、Automation Run 4 三项 Success 及用户明确确认的 PIE 激活/重复命中/结束状态分支；保存可提交的原始日志摘录，更新接入/测试/进度和阅读快照，按模板准备报告 | 用户确认三项 Automation 与上述 PIE 项均通过，并要求本人操作 PR、助手仅提供报告 | 日志执行日期为 2026-10-07，归档日期为 10/8；当前实现源码为 c0a7589，保留此前失败历史；当前 T06 范围通过，完整 TC12 随 T07/T08/T13 补测；本轮仅本地归档与静态检查，未代为编译、提交/推送、创建 PR 或修改 GitHub 状态 |

## 2. 本轮归档说明

| 后续变更 ID | 日期 | 内容与原因 | 影响与验证 |
|---|---|---|---|
| CHG025 | 2026-10-08 | 用户确认 T07 六项 Automation 与基础拾取/放下/投掷 PIE 通过；随后明确其他边界目前无法测试，归档局部结果与模板报告 | 保留历史增量构建结果和真实通过范围；完整边界仍待执行，见 [T07 接入](Device_Interaction_Setup.md) 与 [原始日志](../06_Test_Doc/Evidence/T07_Verification_2026-10-08.log) |
| CHG026 | 2026-10-08 | 用户要求直接继续 T08，并授权每完成小功能本地提交；P01 `35643dc`、D03 `e969ac3`、蓝图默认半径/缩放出生检查加固 `8ee90e6`，接入统一 Sweep 弹道、世界方向提示、计划发射、出生净空和结束清理，新增六项 Automation 源码 | 参数 v0.10 新增未实测几何/视觉默认值；继续在 develop 开发，无新分支、推送、PR 或 GitHub 状态变更。助手仅源码/API 复核与静态检查，UE 构建、Automation、PIE/G1 全部待执行，见 [T08 接入](Emitter_Setup.md) 与 [模板报告](../06_Test_Doc/T08_Report_2026-10-08.md)；T07 边界不据此通过 |
| CHG027 | 2026-10-08 | 玩家迁移至 Murdock：单 Jog 速度与参数同步，取消走路/冲刺切换；一体枪使用角色枪口 Socket，隐藏独立枪且保留原 Rifle 玩法；补充 [迁移指南](Murdock_Setup.md) 与外部枪口回归测试源码 | 用户要求人物/枪械一体接入、修正左键无效果并统一 Jog；旧 WeaponSocket 校验会阻止绑定，玩家资产仍引用 Mannequin Montage | 仅代码、参数与文档修改；二进制资产由用户配置，助手未编译/运行 UE，Automation/PIE 待验收；不据旧模板结果标记新模型通过 |
| CHG028 | 2026-10-11 | 收尾 T09：新增九项原生玩家边界测试，复现并修正无 Flush 应用失焦续射与 UnPossessed 后旧角色续射，清理解除控制时的换弹/恢复/Held；初始化释放查询输出以消除 C4701 | 用户要求收尾 T09 / #11；当前 Issue 前三项已勾选，末项与验证记录未完成 | 助手实际执行 UE 5.8 Editor 增量构建与全量 23 项 Automation，全部 Success、未预期 errors/warnings 为零；保留首轮两项失败。玩法参数和用户资产未改，真实 Murdock/窗口/整局重开仍待证据。源码与文档本地提交，未推送或修改 GitHub，见 [T09 报告](../06_Test_Doc/T09_Report_2026-10-11.md) |

T05 接入补记（2026-10-03）：用户已创建 `/Game/MothEffect/Weapons/BP_Rifle`，并在 `hand_r` 下添加 WeaponSocket；C++ 默认挂点与 Weapon_Setup 已同步，已有角色蓝图仍需核对挂点字段。助手未修改资产或执行编译/试玩。

CHG027（2026-10-08）：用户提供 T08 Devices Test Run 3，九项 Success，三个 Emitter 案例带缺少 World EndPlay 警告；读取并归档本机原始日志与 UBT 的 up-to-date 成功检查。提交 `23eb829` 用 UWorld::EndPlay 配对世界/Actor/Subsystem 收尾，修正后编译与复测待用户执行；同步 [接入步骤](Emitter_Setup.md)、[报告](../06_Test_Doc/T08_Report_2026-10-08.md) 与 [验证日志](../06_Test_Doc/Evidence/T08_Verification_2026-10-08.log)。P01 三项、PIE 和 G1 仍待验证；助手没有运行 UE，用户资产改动保留。

阅读指南归入项目概览；游戏策划、道具规则、关卡/界面/资源和参数归入 GDD；UE 实现设计归入 TDD，并拆出代码规范、版本管理和工程配置；测试与提交拆为测试计划、发布清单；决策与变更拆为设计决策、维护流程和本变更日志。根目录 README 提供有效文件索引，阅读版同步为新结构。

CHG028（2026-10-08）：归档用户 17:06 最新 Test Run 3，Devices 九项与 Projectiles 三项全部 Success，World 清理警告不再出现。保留 16:43 的旧警告历史与两轮 UBT up-to-date 检查，更新 T08 报告、接入和任务/测试状态；本轮仅记录实际结果与同步阅读版，未更改玩法源码或用户资产。PIE TC03–TC05、真实暂停和 G1 仍待验收；助手未运行 UE。

早期 v0.1/v0.2 文件包是历史快照；当前有效基线以项目 Docs 为准。

CHG029（2026-10-08）：用户报告 PIE 中没有发射方向提示；核对 UE 5.8 Arrow 视图相关渲染条件和旋转矩阵，修订接入为 EmitterDirectionMarker 下的 DirectionVisual 锥体网格，明确局部 Pitch=-90°、相对挂接、材质/显隐和激活后显示。原生方向/显隐及玩法源码未改，十二项 Automation 通过记录保留；配置后的 PIE 效果待用户确认。助手未修改资产或运行 UE。

CHG030（2026-10-08）：用户随后确认方向显示通过并询问 PR；归档该局部 PIE 结果，更新 T08 模板报告与阅读版，保留完整 TC03–TC05/G1 待验收。用户已将四项蓝图/重定向器资源纳入 `e44a44a`，LFS 属性与指针已核对；磁盘资产未检出 Cone/DirectionVisual，最终方向配置存盘仍需核对。助手仅本地提交文档，未创建分支、PR、推送或执行 UE。

CHG031（2026-10-11）：用户在 T09 PIE 收尾清单后确认“测试通过”，归档确认原文和当前可执行分支范围。补齐 Issue #11 最后一项验收及实际验证记录，关闭任务并同步 Project 为 Done；更新报告、任务/测试/技术设计、文档入口和阅读快照。此前 Editor 构建与 23 项原生 Automation 通过结果沿用，未重复执行 UE；正式胜负/重开继续由 T16 联调，完整 G1/G3 不随本次关闭通过。本轮文档仅本地提交，BP_MothPlayer 工作区改动保留给用户，推送及 PR 由用户操作。

CHG032（2026-10-11）：用户要求继续 T10；`5692d76` 接入单次爆炸、固定爆心、静态共同遮挡、Actor 去重与统一帧末推力，补齐死亡/禁用后的待消费 Launch 清理。UE commandlet 生成并保存 BP_DeviceBomb 引擎 Sphere 灰盒，参数 v0.12 新增 0.25s Spent 清理。Editor 构建及最终 32 项 Automation 全部 Success，九项新增含实际 World/运动和保存资产重新加载；初次编译/脚本修正记录保留。新增接入、验证报告和证据并同步阅读版，#12 为 Open / In Progress，图形 PIE、正式敌人、D01/完整连锁与 G2 待验收；只本地提交，用户资产改动保留，未推送或创建 PR。

CHG033（2026-10-11）：用户在 T10 当前 PIE 清单后确认“测试通过”，归档原文和用户报告的证据类型，补齐 #12 验收记录并关闭，Project 同步为 Done。更新接入、报告、任务/测试/技术设计、参数说明和阅读快照；正式敌人/掉落、实际 D01 组合、完整连锁和 G2 随后续任务验收。沿用 `5692d76` 的 Editor 构建和 32 项 Automation 结果，本轮未更改玩法或重复运行 UE；文档仅本地提交，用户自行推送及创建 PR。

CHG034（2026-10-11）：用户要求继续完成 T11；`a5903f6` 接入世界 -Z 承托探测、最终体积净空、ALaunchColumn 伸出/停留/收回、单次目标弹射、占用净空后实体支撑与关闭支撑时主动释放站立角色。DEC30 先记录灰盒方形截面、斜面最高接触、中心探地边缘与底部容差；参数 v0.13 补标签、1cm 容差和 1s Spent 反馈。保存 BP_DeviceLauncher / BP_LaunchColumn，引擎资源与 LFS 已核对；Editor 构建及最终 43 项 Automation 全 Success，11 项新增含真实运动/站立、D01/D02 顺序与保存资产加载。首轮 41/43、中间重载编译问题和修正日志保留。#13 为 Open / In Progress，当前真实角色 PIE、正式敌人/完整连锁和 G2 待验收；新增接入/模板报告/证据并同步阅读版，仅本地提交，用户资产改动保留，未推送或创建 PR。

CHG035（2026-10-11）：用户在 T11 探地碰撞排查后确认“通过 提交并pr”。归档用户确认与 15:19 PIE 的三次成功起柱/正常种子结束（含空中启动），保留此前 NoGround / UnsupportedSurface 和 ABP_MothMurdock 除零警告；接入明确 BlockAll 为 WorldStatic、BlockAllDynamic 为 WorldDynamic，不改变现有承托规则。#13 验收补齐，关闭并同步 Project Done；更新接入、报告、任务/测试/技术设计与阅读快照。沿用既有 Editor 构建与 43 项 Automation 结果，本轮仅文档及 GitHub 收尾，没有新增助手图形 PIE 执行。按本次用户授权提交、推送 develop 并向 main 创建包含 T10/T11 的 PR；用户已有 BP_MothPlayer 资产改动不纳入，正式敌人/掉落、完整连锁、独立包及 G2 仍待验收。

CHG036（2026-10-11）：用户要求继续推进 T12 / #14；`bc3ce46` 交付十项真实 World Tick / Chaos / P01 Sweep 连锁测试、布局阴性对照、可选 P01 spawn/impact/expired 日志，以及五组布局/14 件普通装置的 L_T12Chains 和创建脚本。DEC31 先约束只启动起点，不直接激活/命中后续目标或在连锁中传送对象；玩法参数 v0.13 不变。最终 Editor 构建和全量 53 项 Automation Success，首次 7/9、观察窗口修正、编译/脚本修正及最终结果分别归档。新增接入、报告与脱敏证据，更新任务/测试/技术设计/入口/阅读快照；#14 为 Open / In Progress，当前 Murdock 图形 PIE、正式敌人/完整 G2 仍待验收，仅本地提交，不推送到此前 T10/T11 PR #39，用户原有玩家资产改动保留。

CHG037（2026-10-11）：用户授权“可以 创建pr”，随后明确验收回复“全部通过，可以记录验收”。按用户确认归档 T12 五站连锁/布局对照、暂停恢复与结束清理；沿用此前 Editor 构建及全量 53 项 Automation，不新增助手图形 PIE 或录屏声明。#14 验收补齐并关闭，Project Done；更新接入、报告、设计状态、任务/测试/入口与阅读快照。T10/T11 PR #39 已合入 main，保持本地 develop，T12 独立推送远程 codex/t12-natural-chains → main 并按模板创建 [PR #40](https://github.com/YYchainsAw/MothEffect/pull/40)；只包含 T12 已提交内容，用户原有 BP_MothPlayer 改动保留。正式敌人/掉落、容量、整局重开、独立包和完整 G2 仍随后续任务验收。

CHG038（2026-10-11）：用户要求继续推进完成 T13 / #15。新增 World-local 容量登记、掉落总额预占、Active 最早启动安全替换、P01共享额度和即时释放、累计闲置/警告/Spent0.25s、显式/世界界外与拒绝旧世代/回调生成的重置清理；修正已Spent种子直接销毁时派生柱的最终清理。DEC32和参数v0.14记录运行时Config副本；新增十三项原生测试、Development控制台及接入/模板报告。初次C++编译完成但原DLL被当前编辑器占用而链接失败，不强制关闭或热重载；在Saved临时工程复制并核对相同源码、Content目录引用只读加载，完整隔离Editor构建通过，全量66项Automation Success（新增13/既有53），导出errors/warnings为零；125个源码文件和全部Config与当前工程逐字节核对一致，原工程DLL仍需关闭旧编辑器后完成正式链接。#15保持Open / In Progress，当前角色PIE、正式敌人/完整整局和G2待验收；只本地提交，不推送或创建PR，用户原有资产改动保留。

CHG039（2026-10-11）：用户确认已保存并关闭编辑器，继续完成T13原工程验证。首次重编译发现Unity合并测试的Floor辅助函数重名，62e6156改名LifecycleFloor；保留失败15.25s及旧DLL锁定历史。原工程Development Editor / Win64正式Unity编译/链接Succeeded（13.33s），新DLL在原工程UnrealEditor-Cmd / NullRHI进程中加载，于19:04:46香港时间完成全量66项Automation Success，新增13/既有53均通过，每项errors/warnings与失败/未运行计数为零。八项既有启动warning与构建弃用warning分别记录；玩法参数和用户玩家资产哈希未变。当前PIE尚未确认，#15仍Open / In Progress；同步接入、报告、设计/参数证据、进度和阅读快照，只本地提交，未推送或创建PR。

CHG040（2026-10-11）：用户在T13五项验收清单后确认“测试通过 创建pr”。按用户报告归档当前Murdock PIE的过期/累计持有放下投掷、暂停冻结、总容量回收保护、效果/弹丸结束与重复清理/重新PIE，结合原工程Editor正式构建及全量66项Automation，补齐#15验收并关闭、Project Done。快速七件Active、64发饱和/恢复与显式界外保留原生证据，不新增助手图形PIE或录屏。本轮只更新验收/参数证据/任务/设计/报告和阅读快照，不改玩法数值或重复运行UE。按本次授权向远程codex/t13-lifecycle推送并以main为base创建独立T13 PR，本地继续develop，用户BP_MothPlayer改动不纳入；正式敌人、完整整局/跨波、最终场地、独立包与完整G2继续随后续任务验收。

CHG041（2026-10-11）：已按用户授权推送T13已提交内容至远程codex/t13-lifecycle，向main创建[T13 PR #41](https://github.com/YYchainsAw/MothEffect/pull/41)，当前Open；PR按仓库模板记录原工程构建、66项Automation与用户当前PIE验收，#15为Closed / Project Done。首个PR head为d486fbc，后续仅同步实际PR链接和交付记录，UE验证源码仍为62e6156；Repository checks按GitHub实际head结果核对。PR差异21件代码/配置/文档文件，无UE二进制资产，本地develop未切换且已有BP_MothPlayer改动保留。本轮没有授权合并PR，合并状态按GitHub后续结果记录。

CHG042（2026-10-11）：用户要求推进T14，并提醒开发过程提交。e201be2交付一套射击EnemyBase、共用Health/Impulse/P01、视线/预警/间隔、最后可见位置、静态导航重试/停滞换点和死亡事件，DEC33/参数v0.15同步。1ad8496保存BP_EnemyBase与L_T14AI并接入首轮测试/Development命令；首轮新增8/10，步枪许可和道具遮挡夹具按正常玩家按键与已发弹路径修正，f68badf保留绕行目标跨重寻路。首个commandlet导航重载因AsyncLoadLock无有效路径，真实Editor Tick后正式Build/保存并验证绕静态掩体；另一个实际保存场地Game进程2项导航通过，含临时实体挡路实际侧向恢复。f68badf全量76 Success + 1 Success with warnings，原生玩家刻意无可播放动画导致NormalRifleKillsOnce带一条动画警告，玩法断言通过；678dc66明确该玩家恰好一次预期警告，不改变玩法断言。最终Editor构建Succeeded（16.18s），21:04:19香港时间全量77项Editor Success（新增11/既有66）、21:02:10两项Game Success，每项errors/warnings=0。记录首轮失败/导航锁及既有进程warnings，新增接入/模板报告/脱敏证据并同步阅读版；#16保持Open / In Progress，当前图形PIE与T15掉落/T16整局/T17最终场地及完整G3待验收。本轮逐功能仅本地提交，不推送或创建PR，用户原有玩家资产改动保留。

CHG043（2026-10-12）：用户要求创建T14 PR，明确当前PIE“尚未完成，保留待验收”。核对T13 PR #41已合并，T14相对main差异26件文件且不含既有玩家资产改动，保持本地develop，推送独立远程codex/t14-shooting-ai并创建[T14 PR #42](https://github.com/YYchainsAw/MothEffect/pull/42)，当前Open。首个head为1a18890，Repository checks run38189638920已Success；原工程Editor构建/77项Editor与2项Game导航验证源码仍为678dc66，未重复运行UE。两件新LFS资产的三个历史版本已上传，LFS fsck、本地静态与36节阅读检查通过。同步实际PR链接、用户待验收记录、交付状态和阅读版；#16保持Open / In Progress，合并后仍未验收再转Pending Acceptance；不自动关闭或合并。本轮文档head的CI另按实际结果核对。

CHG044（2026-10-12）：2026-10-12 T15推进：三个BP_EnemyBase子类以青色三角棱柱D01、橙球D02、紫长条D03预告固定掉落，共用T14射击AI；生命死亡后只尝试一次，近处有效出生、真实物理Dormant、24额度回收/保护与回调世代取消统一接入。保存12件新资产与L_T15Drops静态导航，参数v0.16/Config和DEC34同步。原工程Editor构建通过，全量88项Editor Success（新增11/既有77）；仅测试夹具修正后新增11项再次Success、2项Game保存导航Success，导出errors/warnings=0。T15 #17保持Open / In Progress、当前Murdock图形PIE待验收；T14 #16仍待用户PIE，PR #42未追加本轮内容。本轮develop本地提交，无T15推送/PR。以下段落保留历史。 核心f8e3216，资产/原生53ba114，最终测试6512650；测试接入15.38s、最终Editor构建14.98s。全量88项09:27:20通过；仅测试源码修正后新增11项09:32:35与Game导航2项09:32:32通过。保留OBJ UV Ensure、Tint API返回值、保存地图导航初始化/已有地图拒绝与额外射手胶囊挡视线失败证据；不改变生产视线规则。新增接入/报告/脱敏证据与38节阅读版，#17待当前PIE，未创建T15 PR。

CHG045（2026-10-12）：2026-10-12 T15 PR记录：用户要求“先保持待验收 创建pr”，已推送codex/t15-enemy-drops并创建[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43)，当前Open。T14 PR #42尚未合并，T15暂以codex/t14-shooting-ai为目标，只显示T15的36件改动，待#42合并后再改为main。#17继续Open / In Progress，当前图形PIE待验收、验收框保留未勾选；不关闭任务。原工程Editor构建、88项全量与最终11项复测/2项Game导航沿用已归档实测，本次仅PR与文档记录，未重复运行UE。本地继续develop，玩家原有资产改动未纳入。现有workflow仅自动检查main/develop目标，叠加PR按最终head手动运行Repository checks，实际结果另以GitHub检查页为准。以下段落保留历史。 PR首次head为0f3efc344907cb376c4e03da51af085c721f89e8，12件新LFS对象共375KB已上传；36件改动与用户玩家蓝图排除范围核对，本地241件文件/38件UE资产、38节阅读快照、diff和LFS fsck通过。同步PR链接、目标分支与用户待验收记录，不自动合并或关闭#17。

CHG046（2026-10-12）：[T15 PR #43](https://github.com/YYchainsAw/MothEffect/pull/43) 的PR记录head da6ff26已手动触发[Repository checks run38194066109](https://github.com/YYchainsAw/MothEffect/actions/runs/38194066109)，10:00:19香港时间Success，static-checks 6s；现有workflow目标分支过滤使自动PR检查未触发，手动结果在Actions运行页。同步实际CI类型/链接与阅读版，不修改当前PIE待验收状态；本次文档归档后的最终head另行核对GitHub实际运行结果。

CHG047（2026-10-12）：2026-10-12 T16推进：三波导演已接入4 / 3+3 / 4+4固定种类/出生点，准备与延迟批次均计入待生成；真实胶囊/静态导航/安全距离检查和完整预警，占位后只在原点重试。玩家死亡/界外判负，末波结束胜利，World-local开关先关闭伤害/外力/生成再清对象与任务/绑定；当前地图OpenLevel重开，暂停结果页可重开，跨波不刷新Dormant闲置。保存BP_WaveDirector、原创警戒材质和L_T16Flow三件资产及八点静态导航，参数v0.17/Config和DEC35同步。Editor构建、最终全量95项Editor、5项真实Game流程与2项Game导航Success；实际4/6/8出生及8存活峰值、批次空窗、物理预警中断、界外、跨波闲置和十次全新World重开有证据。最终Editor导出errors=0 / warnings=1（引擎联网探测超时），此前94项warnings=0；Game流程errors=0，既有Paragon/Murdock加载/动画等156条warnings如实留存。#18保持Open / In Progress，当前图形PIE待验收；T14/T15仍待验收，T17最终场地、T18完整UI/教程及独立包/G3继续对应任务。本轮develop本地提交7d4edba/8932848/dca4554，未推送/创建T16 PR，用户玩家资产未纳入。以下段落保留历史。 接入/报告/脱敏证据新增，阅读版扩为40节。保存及首轮Game失败分别归档；已验证的终态与重开不替代T18完整界面、独立包或用户验收。


CHG048（2026-10-12）：用户确认合并完成，核对PR42/43/44已合并main并以cbe4330同步本地develop，保留玩家资产。完成T17主竞技场L_Arena_01、八件新LFS包与布局源/生成脚本，默认关卡切换，15条保存重载路径通过；新增三项场地测试、复用五项流程测试验证主地图，原工程Editor构建27.15s与8项Game全部Success，errors=0 / warnings=156；三项新场地测试无告警。先提交d9b3851/dce5cc6，再归档报告/证据/DEC36/配置/TC17/21/32与四组当前PIE清单。保留旋转轴、已有地图NewLevel拒绝及路径中点检查三次失败；修正后真实步行/P01通过。参数v0.17未改，无T17/T16推送或PR，#19保持Open / In Progress，当前图形PIE及独立包/完整G3待验收；已有PR合并不扩大之前手动验收范围。


CHG049（2026-10-12）：2026-10-12 T16/T17 PR记录：按用户要求“创建pr”“保持待校验”，已推送codex/t16-t17-wave-arena → main并创建[PR #45](https://github.com/YYchainsAw/MothEffect/pull/45)。main尚无T16，因此同一PR交付三波/胜负/重开及T17主竞技场，共51件功能/证据改动、11件新LFS资产；使用Refs #18/#19，两项Issue继续Open / In Progress，四组图形PIE分别待校验、完整验收框未勾选。原工程T16最终95项Editor/5项Game及T17主场地8项Game沿用既有真实记录，本次仅PR与文档，不重复执行UE。独立PR工作树验证271件文件/49件UE资产、42节阅读版、diff及LFS fsck通过；11件LFS对象220KB已上传，用户玩家蓝图未纳入。本地继续develop，GitHub Repository checks按最终head另行核对，静态CI不代替游戏验收。以下段落保留历史。 首次PR head c99c2b2241c8d5a9d3f773e2d581332245ecaa32，main基线0780180；PR模板完整列出构建/原生结果、既有156条Game告警、14条缺失依赖包报告、未提交本地玩家资产和外部依赖恢复/独立包边界。当前PR不是已合并/已验收，最终head的GitHub检查结果另看PR页。

CHG050（2026-10-12）：2026-10-12 T18推进：原生 UMG HUD、三种原创矢量图标、实际动作驱动的三步教学和标题/暂停/教学完成/胜负/重开面板已接入。首次读取真实生命/弹药/持物/波次，事件更新并在换玩家/重开时解绑；进入战斗重载关卡，仅保存一次开始意图。教学分别核验爆炸死亡来源、真实物理抬升、投出且空中 W01 启动、D03 P01→D02 间接击杀；落地/错误启动补充，独立目标允许换顺序，暂停保留合法换弹。实现阶段96项 Editor 全量及主地图10项 Game 回归通过，后续教学重试/最终布局的 UI 分支在三种分辨率真实 D3D12 离屏渲染复测通过；既有资源/动画告警保留。#20 Open / In Progress，五组当前硬件输入与完整图形 PIE 待验收，TC26 外部盲玩及独立包留后续；本轮本地 develop 提交，未推送/创建 T18 PR，T16/T17 PR #45 待校验状态保留。 参数账本 v0.18 仅登记新教学阈值/提示时长的原生常量，既有战斗数值不变；DEC37、接入、TC 分支、截图与失败/告警证据同步，用户玩家蓝图未改写或纳入提交。

CHG051（2026-10-12）：2026-10-12 T18 PR记录：按用户要求已推送 codex/t18-hud-tutorial → main 并创建 [PR #46](https://github.com/YYchainsAw/MothEffect/pull/46)。创建期间核对到 T16/T17 PR #45 已合并至 main 8481463，因此 T18 直接面向 main；合并本身不扩大前轮手动验收范围。PR 的 Assignee 为 YYchainsAw，Labels 为 feature / area:ui，Project 为 MothEffect · 首版开发（Status In Progress），Milestone 为 M4 · 三波可玩闭环，均继承 #20 并在 GitHub 回读核对。使用 Refs #20，Issue 继续 Open / In Progress、六项完整验收框未勾选、五组当前硬件 PIE 待验收。实现阶段96项 Editor/主地图10项 Game 和最终三分辨率 UI 沿用已归档证据，本次 PR/元数据/文档未重复 UE；独立 PR 工作树297文件/50UE资产、44节阅读版、diff与LFS fsck通过。原开发与PR初始交付的完整 Git tree 相同，用户玩家蓝图未纳入。本地继续 develop；最终 head 的 Repository checks 另按实际 GitHub 运行核对。 接入/报告和GitHub流程补充PR元数据核对要求；Git连接和旧base删除的失败均先回读远端，再执行必要重试；只创建一个T18 PR，不重复开单。

CHG052（2026-10-13）：T18编辑接入按用户要求完成代码与基础骨架，具体UI由用户制作。WBP保存54控件/25必需BindWidget，四按钮按构造/销毁绑定；教学面板脱离固定父子层级，字体/位置/普通样式由Designer管理。Class Defaults提供菜单文案/页面与持物预览，DA_MothTutorial提供教学说明/反馈/布置，Subsystem复制本轮Transient配置快照。Editor构建、保存/新进程重载、2项编辑回归、主地图10项与扩充UI2项通过；50条临时空蓝图提示、205/48条Game告警及Build2测试字号类型失败如实归档。DEC38、[制作入口](UI_Setup.md)、[T18报告](../06_Test_Doc/T18_Report_2026-10-12.md)、证据与阅读版同步；PR #46已合并并同步develop，本轮106b7bd/eefd06d本地提交，未推送/创建新PR；用户玩家资产保持原哈希且未纳入。#20保持Open，五组硬件PIE、独立包与外部盲玩继续待验收。

CHG053（2026-10-13）：按用户要求完成T19默认反馈。实现8112648接入无碰撞网格、机关类型/状态/过期轮廓、D03世界方向、命中/受伤屏幕反馈和8个原创合成短音效，DA_DefaultFeedback提供十二槽与预算/音量/时长配置，共11件保存资产新进程重载成功。修正e00348f加固世界轴向、箭头端点、最旧实例淘汰与出生观察者重入清理，补MothFeedbackStats和6项Editor/1项Game测试；真实渲染发现T18共享字体来源重载缺失，只补默认来源并保留用户有效字体和样式。最终构建6.84s、104项Editor回归及最终9项反馈/UI、D3D12/Realtime音频主地图11项通过，最终errors=0，临时空蓝图50条/既有Game220条warnings如实保留；首轮告警期望、渲染夹具误用真实时间和失败构建均归档。五张原始1920×1080截图按LFS保存，DEC39、[制作入口](Feedback_Setup.md)、[报告](../06_Test_Doc/T19_Report_2026-10-13.md)与46节阅读版同步。#21 Open / In Progress，四组真实声音/遮挡/四向空中/暂停重开待用户验收；独立包和外部盲玩留T21/T22。直接develop本地提交，未推送/创建PR，用户玩家蓝图未改写或纳入；参数v0.18和此前待验收范围保留。

CHG054（2026-10-13）：2026-10-13 T20原型完成：三种原创机关外壳、金属掩体模块、符文灯饰、共用材质与DA_PrototypeArt共17资产保存并新进程重载，重复运行文件SHA保持一致。机关与敌人背包共用可替换外观，主场地九块掩体与三灯饰由可关闭场景层接入；现有碰撞/导航和v0.18玩法基线保留。构建、107项全量Editor回归及最后一项夹具修正后的3项专项、实际D3D12/音频主地图12项通过；首轮围墙误识别、Static夹具、旧灰盒断言与导入失败如实留档。用户后续深度修改机制与整套UI，当前实现作为可运行基线；五组真实玩家/场地/独立包待验收，第三方缺依赖与完整恢复待补。实现3c6c8e5、Refs #22本地提交，补[原型制作入口](Prototype_Art_Setup.md)、[报告](../06_Test_Doc/T20_Report_2026-10-13.md)、三张原始1920×1080截图、DEC40与48节阅读版。初始TObjectPtr/成员遮蔽构建失败、OBJ槽排序/Y镜像导入失败及两轮测试失败保留；新构建发现T19上升线Rotation未初始化并修正。#22保持Open / In Progress、六验收框未勾选；用户玩家资产未保存或纳入，未推送/创建PR。

CHG055（2026-10-13）：2026-10-13 已按用户要求创建[PR #47](https://github.com/YYchainsAw/MothEffect/pull/47)：T18编辑入口、T19默认反馈与T20可替换原型外观，codex/t18-t20-prototype-feedback → main。base ce3a887、首个head 3a7d837，完整Git tree与已验证develop cadff96一致；126件差异，46件LFS对象11MB已上传，用户玩家资产未纳入。Assignee YYchainsAw，Labels feature/polish/area:ui/area:combat/area:art，Project MothEffect · 首版开发 / In Progress，Milestone M5 · 试玩与候选包验收，均已回读核对。首个head的Repository checks已Success（6s）；静态CI不代替游戏验收。使用Refs #20/#21/#22，关联Issue保持Open，各轮手动验收保留。本地继续develop；本轮PR沿用既有真实UE证据，随后T21独立包另行记录。 网络失败先回读确认未创建重复PR，改用REST完成创建，再补字段；仅一份PR，未自动合并或关闭任务。
