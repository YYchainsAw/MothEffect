# Moth Effect（飞蛾效应）：GitHub 开发流程

版本 v0.1 · 2026-10-06 · 香港时间（UTC+8）

仓库：[YYchainsAw/MothEffect](https://github.com/YYchainsAw/MothEffect)；看板：[MothEffect · 首版开发](https://github.com/users/YYchainsAw/projects/1)（Private，需要项目权限）。阶段截止和任务进度见 [开发任务与排期](Development_Plan.md)，游戏实测标准见 [测试计划](../06_Test_Doc/Test_Plan.md)。

## 1. 从任务到验收

1. 从看板中选择依赖已成立的任务，设为 `In Progress`，按 Issue 的范围开发。计划任务沿用 T01–T27；新增工作使用具体标题，需要正式纳入计划时再维护任务编号。
2. 从 `develop` 创建工作分支，在 PR 中写改动目的、验证结果及 `Refs #实际编号`；一个 PR 可关联多个 Issue，也可分项说明功能、修复和文档改动。
3. 确认 PR 的 `Repository checks` 通过，再合并。日常游戏开发以 `develop` 为集成分支，验收后的阶段版本再进入 `main`；本次仓库配置 PR #30/#31 已进入两条分支。
4. 游戏任务合并后进入 `Pending Acceptance`，补齐受影响的 UE 测试分支及证据。符合 Issue 验收条件后设为 `Done` 并关闭 Issue；文档或仓库任务记录相应检查结果即可。

需要合并后验收的任务使用 `Refs`，避免自动关闭关键词跳过验收。GitHub 的 Issue 开关状态与 Project 的 Status 分别记录；若出现差异，先核对证据，再同步状态。2026-10-06 核对到 T04 为 Closed / Pending Acceptance，详见开发计划。

## 2. 新建 Issue 与 PR

[新建 Issue](https://github.com/YYchainsAw/MothEffect/issues/new/choose) 时可选：

| 模板 | 用途 | 主要填写内容 |
|---|---|---|
| [开发任务](../../.github/ISSUE_TEMPLATE/task.md) | 功能、改进、文档与工程工作 | 目标、依赖、T/TC/规则 ID、验收标准、实际验证与证据 |
| [游戏 / 工程 Bug](../../.github/ISSUE_TEMPLATE/bug_report.md) | 可复现的问题 | 版本与环境、步骤、频率、预期/实际、S0–S3、修复及回归；自动使用 `bug` 标签 |

创建时选择现有 Labels、负责人、Milestone 和 Project。标签描述工作类型或模块，可同时添加多个；S0–S3 严重度按测试计划填写，不依靠标签推断。

新建 PR 自动使用 [默认模板](../../.github/pull_request_template.md)，填写关联 Issue、实际执行的检查及待验收项。未执行的测试如实保留；不把代码存在或 CI 通过写成游戏测试通过。Issue/PR 的 `#编号` 由 GitHub 按创建顺序统一分配，与文档里的 T 编号分别使用。

## 3. Project 的四个视图

打开看板后选择对应视图；各视图显示同一批项目条目，筛选不会复制任务。

| 视图 | 布局 | 当前筛选 | 日常用途 |
|---|---|---|---|
| [开发看板](https://github.com/users/YYchainsAw/projects/1/views/1) | Board | 全部条目 | 按 Status 推进任务 |
| [当前阶段](https://github.com/users/YYchainsAw/projects/1/views/2) | Board | `is:issue is:open milestone:"M1 · 可运行的射击基础","M2 · 投掷与空中激活"` | 聚焦未关闭的 M1/M2 任务 |
| [待验收](https://github.com/users/YYchainsAw/projects/1/views/3) | Table | `is:issue status:"Pending Acceptance"` | 查找需要补测、留证的任务，包括已关闭但仍待验收的条目 |
| [发布检查](https://github.com/users/YYchainsAw/projects/1/views/4) | Table | `is:issue milestone:"M5 · 试玩与候选包验收","M6 · 最终包与提交"` | 核对试玩、候选包及提交工作 |

四个视图显示 Title、Status、Assignees、Labels、Milestone。推进到 M3 后，把“当前阶段”的 Milestone 筛选改为当时实际需要跟踪的阶段；筛选中的阶段不会按日期自动切换。

## 4. 轻量 Actions 的检查范围

[Repository checks](https://github.com/YYchainsAw/MothEffect/actions/workflows/repository-checks.yml) 在推送 `main`/`develop`、提交目标为这两条分支的 PR 时运行，也可手动触发。工作流定义见 [repository-checks.yml](../../.github/workflows/repository-checks.yml)，检查实现见 [validate_repo.py](../../scripts/validate_repo.py)。

- 检查跟踪的 JSON、`.uproject`、`.uplugin` 是否有效，拒绝重复键及非标准数值常量。
- 检查根 README 与 Docs 中 Markdown 正文里的行内相对文件链接；不校验网页链接或页内锚点。
- 检查跟踪的 `.uasset` / `.umap` 是否配置 LFS 属性；不下载 LFS 内容，也不验证远程资源完整性。

本地在工程根目录运行 `py scripts/validate_repo.py`。Actions 不编译或运行 UE，不打包游戏，不验证机关规则、动画、性能或平台审核；这些结果按测试计划记录。

## 5. 同步 Docs

规则、接口和参数在各自权威文件维护，Issue/PR 引用它们。任务状态、排期或验证发生变化时，更新 [开发计划](Development_Plan.md) 和对应接入/测试记录；在 [变更日志](Changelog.md) 中保留关联 Issue/PR。

先更新 Markdown/JSON 源文件，再按 [文档维护流程](Documentation_Workflow.md) 同步日期、链接和 HTML 阅读快照。玩法参数仍以参数账本为唯一文档基线，已记录的代码差异需要随实际调参结果归档。

在工程根目录运行 `py scripts/build_reading_view.py` 更新快照（需要 `gh` 已登录）；用 `py scripts/build_reading_view.py --check` 离线核对源文件与快照一致。
