# Moth Effect（飞蛾效应）：项目概览与阅读指南

版本 v0.7　日期 2026-10-03　适用 单人 UE 5.8 C++ 与蓝图混合 GameJam

这套文档把第三人称越肩射击、敌人掉落机关、投掷与空中激活转成可追踪的执行基线。正式英文名为 Moth Effect，中文名为飞蛾效应，美术风格为魔法朋克。用户已经创建第三人称模板工程；本次只读核对了工程声明、源码和资产目录，不编译、运行或修改游戏实现。自定义玩法、性能验证和平台提交仍待后续验收。

## 0 当前工程与文档位置

工程根目录为 `../..`（相对于本文件所在目录），工程文件为 [MothEffect.uproject](../../MothEffect.uproject)，声明的引擎版本为 5.8，运行时模块为 MothEffect。对外英文名统一为 Moth Effect，中文名统一为飞蛾效应；工程名与模块名保留 MothEffect。

工程 Docs 目录保存当前工作文档，目录结构和有效文件清单见 [文档入口](../README.md)。旧交付包保留为历史快照；后续维护以工程内文档为准。当前已有模板角色源码、ThirdPerson 蓝图与关卡；这些文件存在不等于本作越肩射击和机关系统已经完成。

起步建议沿用第三人称模板，复用角色移动、基础动画、输入与相机，再开发本作射击和道具规则。空白工程适用于需要自定义控制框架或已有基础资源的情况；当前单人周期内，切换空白会增加重新搭建的工作。模板中的 Combat、Platforming、SideScrolling 示例不自动纳入本作范围。

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

全部参数的验证状态是未实测；T02 因模板工程已存在、T03 因 Git/origin/忽略/LFS 已配置而标为部分完成，其余开发任务仍未开始；测试全部未执行。文件齐全不代表游戏完成，当前尚无 Git 提交或推送。

## 4 减少返工的四个约束

交互行为以 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 为准，自定义接口以 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 为准，内容数量以 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 为准，数值以 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 为准。其他文档引用它们，不独立维护第二套规则。

先验证空中投掷启动，再验证机关连锁，再制作完整流程，最后扩大视觉完成度。无法验证的手感不靠增加道具类型补救。

G2 之后冻结道具类型和接口，持续调整数值。变化在 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 和 [变更日志](../05_Development_Guide/Changelog.md) 留痕，流程按 [文档维护与变更流程](../05_Development_Guide/Documentation_Workflow.md) 执行，同时重跑关联测试。

每天保留能启动的版本；在第一阶段打出 Windows 小包，提交前依据 [测试计划与验收](../06_Test_Doc/Test_Plan.md) 留下实际证据，并按 [发布检查清单](../07_Release/Release_Checklist.md) 核对发布。

## 5 现在可以先做的工作

下一阶段按 [开发任务与排期](../05_Development_Guide/Development_Plan.md) 核对现有 UE 5.8 模板工程、完成版本管理和首个独立打包，再制作灰盒验证房。然后实现一个发射装置及投掷、空中射击、方向提示，用重复操作验证 G1。相关验收按 [测试计划与验收](../06_Test_Doc/Test_Plan.md) 执行；当前仅确认工程及模板文件存在，没有运行或打包通过结果。

每日可用工时、开发机和最低目标机器还需在开工时填写；[开发任务与排期](../05_Development_Guide/Development_Plan.md) 已按每天四至六小时给出可调整预算，不作工期保证。

## 6 活动依据与日期

本包日期、排期与截止使用香港时间 UTC+8。官方创作时间为 2026-10-01 12:00 至 2026-10-21 12:00，须在截止前上传并通过审核，官方建议 10 月 18 日前上传。本计划把 10 月 17 日作为首版提交目标。[官方主题公告](https://www.taptap.cn/moment/854692229854265392?group_id=792849)

赛事仅支持单机及无网络的本地多人。全程参与奖还要求至少五篇开发日志和至少五十个有效试玩人数，这两项不是全部参赛作品的强制门槛。详细材料与检查见 [发布检查清单](../07_Release/Release_Checklist.md)。[官方答疑](https://www.taptap.cn/moment/851888213915075734?group_id=792849)
