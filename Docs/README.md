# Moth Effect（飞蛾效应）：开发文档入口

版本 v0.16 · 2026-10-07 · 单人 UE 5.8 C++ 与蓝图混合开发

本目录按用户建立的开发阶段结构归档。文件使用稳定的英文语义名称，正文为中文；v0.3 整理结构，v0.4 配置版本管理，v0.5 清理个人路径与发布配置，v0.6 将 Content 的 Git 范围收敛为 MothEffect 子目录，v0.7 统一英文名 Moth Effect 与中文名飞蛾效应，v0.8 新增五种待选机关，v0.9 确认玩家方案，v0.10 开始 T04 C++，v0.11 补充动画接线，v0.12 开始 T05，v0.13 改为角色组件内预览和调整武器，v0.14 同步 GitHub 开发流程、六阶段排期与任务状态。v0.15 核对本机/远程的实际实现，确认每日 4–6 小时，并预留少量原创机关/场景资产和 UI；v0.16 记录 T05 完成并接入 T06 原生机关状态与命中基础。各文件保留自身最近更新版本；文档源文件是 Markdown/JSON，[浏览器阅读版](01_Project_Overview/Reading_View.html) 是汇总快照。

工程：[MothEffect.uproject](../MothEffect.uproject) · 英文名：Moth Effect · 中文名：飞蛾效应 · 风格：魔法朋克。GitHub 仓库已公开并存在提交，`main`/`develop` 已纳入轻量 Actions 和 Issue/PR 模板；Project 与 M1–M6 已建立。日常使用见 [GitHub 开发流程](05_Development_Guide/GitHub_Workflow.md)，提交范围与资源依赖见 [版本管理](03_Code_Standard/Version_Control.md)。

玩家方案已由用户确认：首版不使用 GAS，复用本地 Rifle 动画，采用 C++ 玩家行动状态机、AnimBP 移动状态机与上半身 Montage。开发入口为 [技术设计第 3 节](02_Design_Doc/TDD/Technical_Design.md#3-越肩射击与输入)，依据见 DEC22–DEC27；操作规则为 R19–R23，验收为 TC34–TC38。2026-10-07 最新核对：T01–T03 为 Closed / Done；T04 为 Closed / Pending Acceptance；T05 为 Closed / Done，[PR #34](https://github.com/YYchainsAw/MothEffect/pull/34) 已合并；T06 为 Open / In Progress，原生状态与命中基础已写入，等待用户完整编译和 PIE。完整案例及 G0–G4 以真实证据验收，详见 [开发计划](05_Development_Guide/Development_Plan.md)；[10/7 进度核查](05_Development_Guide/Progress_Audit_2026-10-07.md) 保留当次历史快照。UI 预留 4–6 小时、少量原创资产预留 6–8 小时，必要反馈另留 2–3 小时；修订节点保持 10/17 DDL。

武器预览接入：BP_ThirdPersonCharacter 的 Mesh 下使用 RifleComponent，由子 Actor 组件创建完整 BP_Rifle，在角色视口直接调整组件 Transform；Rifle Class 与 Rifle Attach Socket 保留在角色默认值。T05 Issue 已勾选预览与运行仅一把武器，具体构建与证据仍待补；旧偏移迁移及检查步骤见 [Weapon_Setup](05_Development_Guide/Weapon_Setup.md)。

## 1. 目录与有效文件

| 目录 | 文件 | 职责 |
|---|---|---|
| 01_Project_Overview/ | [Project_Overview.md](01_Project_Overview/Project_Overview.md) | 项目概览 |
| 01_Project_Overview/ | [Reading_View.html](01_Project_Overview/Reading_View.html) | 浏览器阅读版 |
| 02_Design_Doc/GDD/ | [Game_Design.md](02_Design_Doc/GDD/Game_Design.md) | 游戏策划案 |
| 02_Design_Doc/GDD/ | [Device_Interaction_Rules.md](02_Design_Doc/GDD/Device_Interaction_Rules.md) | 道具与交互规则 |
| 02_Design_Doc/GDD/ | [Device_Candidates.md](02_Design_Doc/GDD/Device_Candidates.md) | 候选机关的效果、组合与待验证边界；未进入首版排期 |
| 02_Design_Doc/GDD/ | [Level_UI_Asset_Specification.md](02_Design_Doc/GDD/Level_UI_Asset_Specification.md) | 关卡、界面与资源规格 |
| 02_Design_Doc/GDD/ | [Gameplay_Parameters.json](02_Design_Doc/GDD/Gameplay_Parameters.json) | 玩法参数基线 |
| 02_Design_Doc/TDD/ | [Technical_Design.md](02_Design_Doc/TDD/Technical_Design.md) | 技术设计，含玩家状态/输入/动画/GAS 决策与实施顺序 |
| 02_Design_Doc/TDD/Decisions/ | [Design_Decisions.md](02_Design_Doc/TDD/Decisions/Design_Decisions.md) | 设计决策 |
| 03_Code_Standard/ | [Coding_Conventions.md](03_Code_Standard/Coding_Conventions.md) | 代码与资产命名规范 |
| 03_Code_Standard/ | [Version_Control.md](03_Code_Standard/Version_Control.md) | 版本管理规范 |
| 04_Engine_Config/ | [UE_Project_Configuration.md](04_Engine_Config/UE_Project_Configuration.md) | UE 工程配置 |
| 05_Development_Guide/ | [Development_Plan.md](05_Development_Guide/Development_Plan.md) | 开发任务与排期 |
| 05_Development_Guide/ | [Progress_Audit_2026-10-07.md](05_Development_Guide/Progress_Audit_2026-10-07.md) | 27 条任务的静态实现/资产审计、可复用前置与缺失证据 |
| 05_Development_Guide/ | [Player_Setup.md](05_Development_Guide/Player_Setup.md) | 玩家 C++ 交付范围、输入引用、ABP 数据/移动/瞄准接线与手动验收 |
| 05_Development_Guide/ | [Weapon_Setup.md](05_Development_Guide/Weapon_Setup.md) | T05 步枪/输入/测试靶/上半身动画配置与分支验收 |
| 05_Development_Guide/ | [Device_Setup.md](05_Development_Guide/Device_Setup.md) | T06 原生机关状态、灰盒资产、重复命中与当前验收步骤 |
| 05_Development_Guide/ | [GitHub_Workflow.md](05_Development_Guide/GitHub_Workflow.md) | Issue/PR 模板、Project 视图、分支与轻量 Actions 的实际用法 |
| 05_Development_Guide/ | [Documentation_Workflow.md](05_Development_Guide/Documentation_Workflow.md) | 文档维护与变更流程 |
| 05_Development_Guide/ | [Changelog.md](05_Development_Guide/Changelog.md) | 变更日志 |
| 06_Test_Doc/ | [Test_Plan.md](06_Test_Doc/Test_Plan.md) | 测试计划与验收 |
| 07_Release/ | [Release_Checklist.md](07_Release/Release_Checklist.md) | 发布检查清单 |
| Assets/UML/ | 当前为空 | 后续实际产出的 UML 图及源文件 |

GDD = 游戏设计文档，TDD = 技术设计文档。玩法数值归属 GDD，UE 配置归属 Engine_Config。决策依据与变更历史分开维护；测试与发布也各自独立。

## 2. 按开发流程阅读

1. **目标与范围**：[项目概览](01_Project_Overview/Project_Overview.md) → [游戏策划案](02_Design_Doc/GDD/Game_Design.md)，明确本次要交付的体验和 P0/P1/P2。
2. **行为与内容**：[道具与交互规则](02_Design_Doc/GDD/Device_Interaction_Rules.md) + [玩法参数基线](02_Design_Doc/GDD/Gameplay_Parameters.json) + [关卡、界面与资源规格](02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，明确状态、边界、波次与资源。
3. **技术与环境**：[技术设计](02_Design_Doc/TDD/Technical_Design.md) + [代码与资产命名规范](03_Code_Standard/Coding_Conventions.md) + [版本管理规范](03_Code_Standard/Version_Control.md) + [UE 工程配置](04_Engine_Config/UE_Project_Configuration.md)。
4. **执行与验证**：按 [开发任务与排期](05_Development_Guide/Development_Plan.md) 的 G0–G4 推进，用 [测试计划与验收](06_Test_Doc/Test_Plan.md) 留下实际证据。
5. **变更与发布**：改动遵循 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)，更新 [设计决策](02_Design_Doc/TDD/Decisions/Design_Decisions.md) 与 [变更日志](05_Development_Guide/Changelog.md)，提交时执行 [发布检查清单](07_Release/Release_Checklist.md)。

## 3. 维护约定

目录阶段编号保持现状；文件名用英文语义名称和下划线，不重复加另一套顺序编号。REQ/R/D/T/G/TC/DEC/CHG/RISK 等 ID 不随文件迁移改变。

交互、接口、内容、数值各保留唯一来源，具体分工见 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)。参数账本不是可直接导入 UE 的 DataTable；此前只改代码的移动速度差异仍按 Player_Setup 记录，本次不变更玩法数值。当前任务开关状态、Project Status 与验收证据分别记录，进度快照和 Issue 对照在开发计划维护。

修改源文件后同步更新版本、相对链接和阅读快照；不要只改 HTML。原始平铺文件已在工程外备份，避免在 Docs 中并存两套有效文件。
