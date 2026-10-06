# Moth Effect（飞蛾效应）：项目概览与阅读指南

版本 v0.12　日期 2026-10-06　适用 单人 UE 5.8 C++ 与蓝图混合 GameJam

这套文档把第三人称越肩射击、敌人掉落机关、投掷与空中激活转成可追踪的执行基线。正式英文名为 Moth Effect，中文名为飞蛾效应，美术风格为魔法朋克。T04 玩家基础与 T05 步枪/换弹/生命 C++ 已进入仓库，用户负责 UE 资产配置与验收，助手仍不代为编译。项目用 M1–M6 推进至 2026-10-17 首版提交 DDL，进度和证据见 [开发计划](../05_Development_Guide/Development_Plan.md)，日常协作见 [GitHub 开发流程](../05_Development_Guide/GitHub_Workflow.md)。

## 0 当前工程与文档位置

工程根目录为 `../..`（相对于本文件所在目录），工程文件为 [MothEffect.uproject](../../MothEffect.uproject)，声明的引擎版本为 5.8，运行时模块为 MothEffect。对外英文名统一为 Moth Effect，中文名统一为飞蛾效应；工程名与模块名保留 MothEffect。

工程 Docs 目录保存当前工作文档，目录结构和有效文件清单见 [文档入口](../README.md)。旧交付包保留为历史快照；后续维护以工程内文档为准。仓库包含本作源码与 Content/MothEffect 自有资产；ThirdPerson、Characters、Input 等本机模板依赖的分发和恢复限制见版本管理，不能根据本机曾有这些资产推断新克隆完整。

玩家起步方案已确认沿用现有第三人称模板，复用角色框架、输入与相机，以及本地 Rifle 移动/射击/换弹/瞄准动画。采用 C++ 玩家行动状态机、AnimBP 移动状态机和上半身 Montage，首版不使用 GAS；完整定义见 [技术设计第 3 节](../02_Design_Doc/TDD/Technical_Design.md#3-越肩射击与输入)。T05 的 Ready/Reloading/Dead、Rifle/Health 与组件预览已写入，Carrying/ThrowRecovery 和机关闭环随后续任务实施；模板示例不自动纳入首版。

## 1 文档入口与职责

完整目录和各文件职责见 [文档入口](../README.md)。策划、规则、技术、资源、执行计划、测试与发布分别维护；代码规范、UE 配置与变更流程从原综合文件中拆出。

单人短周期项目沿用这套轻量文档，不额外拆独立世界观案、商业分析或长篇故事案。资源需求、界面与声音见 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)；资源许可、版本恢复和发布要求分别见 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)、[版本管理规范](../03_Code_Standard/Version_Control.md) 与 [发布检查清单](../07_Release/Release_Checklist.md)。

## 2 建议阅读顺序

先读 [游戏策划案](../02_Design_Doc/GDD/Game_Design.md) 确认体验和范围，再读 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 明确机关行为，然后结合 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 实现与 [开发任务与排期](../05_Development_Guide/Development_Plan.md) 的 G0/G1 开始工作。[关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 在安排关卡和资源时使用，[测试计划与验收](../06_Test_Doc/Test_Plan.md) 在完成每个门槛和提交前使用。[设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 保留决策依据，[玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 保留唯一数值基线。

浏览器阅读版汇总上述内容，仅用于方便浏览；修改以对应 Markdown/JSON 文件为准。阅读版为快照，修改源文件后应重新生成。阅读版位于本目录的 Reading_View.html；源文件修改后重新生成，避免展示旧快照。

## 3 状态怎样理解

用户已确认：来自本聊天的明确要求，改变时需要与用户意图一致。

建议基线：为让开发可执行而提供的方案，可以用于可逆原型；不是用户已经同意所有细节。

待原型验证：手感、方向提示和数值等未知，需通过早期实际操作决定。

延期 P2：本次不纳入工时。P1 只有在 P0 和门槛验收完成后才进入。

2026-10-06 核对：T01–T03 为 Closed / Done，T04 为 Closed / Pending Acceptance，T05 为 Open / In Progress，T06–T27 为 Open / Todo。Issue 关闭或勾选项是实际管理记录；构建、测试分支和证据仍需补齐后才能判断完整验收。参数账本仍为未实测基线，G0–G4 的完整通过记录尚未归档；细节统一见开发计划与测试计划。

## 4 减少返工的四个约束

交互行为以 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 为准，自定义接口以 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 为准，内容数量以 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 为准，数值以 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 为准。其他文档引用它们，不独立维护第二套规则。

先验证空中投掷启动，再验证机关连锁，再制作完整流程，最后扩大视觉完成度。无法验证的手感不靠增加道具类型补救。

G2 之后冻结道具类型和接口，持续调整数值。变化在 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 和 [变更日志](../05_Development_Guide/Changelog.md) 留痕，流程按 [文档维护与变更流程](../05_Development_Guide/Documentation_Workflow.md) 执行，同时重跑关联测试。

每天保留能启动的版本；在第一阶段打出 Windows 小包，提交前依据 [测试计划与验收](../06_Test_Doc/Test_Plan.md) 留下实际证据，并按 [发布检查清单](../07_Release/Release_Checklist.md) 核对发布。

## 5 现在可以先做的工作

当前先按 [步枪、换弹与生命接入](../05_Development_Guide/Weapon_Setup.md) 补齐 T05 上半身 Montage 和受影响测试记录，并补档 T01–T04 的环境、构建、恢复及基础动画证据。随后按依赖推进 T06–T09 的机关状态、拾取/安全投掷和 D03 空中启动，达到 G1。已关闭任务不用重建 Issue，缺失记录补在原任务中。

每日可用工时、开发机和最低目标机器的具体记录仍待补。开发计划的每天四至六小时是预算假设，不从 T01 已关闭推断实际时长；剩余预算按当天真实可投入时间重算。

## 6 活动依据与日期

本包日期、排期与截止使用香港时间 UTC+8。官方创作时间为 2026-10-01 12:00 至 2026-10-21 12:00，须在截止前上传并通过审核，官方建议 10 月 18 日前上传。用户已将 2026 年 10 月 17 日设为首版提交 DDL；六个 Milestone 按此倒排，10/16 优先完成首次提交、10/17 处理阻断回归与补提交。[官方主题公告](https://www.taptap.cn/moment/854692229854265392?group_id=792849)

赛事仅支持单机及无网络的本地多人。全程参与奖还要求至少五篇开发日志和至少五十个有效试玩人数，这两项不是全部参赛作品的强制门槛。详细材料与检查见 [发布检查清单](../07_Release/Release_Checklist.md)。[官方答疑](https://www.taptap.cn/moment/851888213915075734?group_id=792849)
