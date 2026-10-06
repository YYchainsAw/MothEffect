---
name: "开发任务"
about: "记录功能、改进、文档或工程任务的目标、依赖和验收"
title: "[Task] "
labels: ""
assignees: ""
---

<!--
已有计划任务沿用 Development_Plan.md 中的 [Txx] 编号，不重排现有 T01–T27。
临时任务可把 [Task] 换成具体标题。
创建时在侧栏选择合适的 Labels、Milestone 和 MothEffect · 首版开发 Project。
按任务填写以下内容；没有依赖填“无”，不适用的测试说明原因。
-->

### 目标与范围

- 目标：
- 包含的改动：
- 依赖 Issue：
- 关联 T / TC / 规则 ID：
- 文档依据：

### 验收标准

<!-- 把第一项替换为本任务可观察的完成条件，按需增加检查项。 -->

- [ ] （替换为本任务可观察的完成条件）
- [ ] 相关测试的适用分支已执行，实际结果和证据已记录

### 验证记录

<!--
实现过程中补充真实记录。尚未执行的测试保留“未执行”。
文档或仓库任务填写实际检查及结果；不需要 UE 测试时说明原因。
-->

- 构建 / commit SHA / 包体标识：
- 参数版本（涉及玩法时）：
- 日期 / 执行人 / 环境（PIE 或独立 Windows 包）：
- 测试 ID / 分支 / 步骤：
- 预期与实际结果：
- 状态：未执行 / 通过 / 失败 / 阻塞 / 不适用（说明原因）
- 证据链接（录屏、截图、日志或检查输出）：
- 待完成分支 / 关联 Bug：
- 关联 PR：

<!--
PR 合并后，仍需 UE 验收的任务进入 Project 的 Pending Acceptance。
验收条件满足并留存证据后再关闭 Issue、设为 Done；静态 CI 通过不代表游戏验收通过。
任务与测试依据：
https://github.com/YYchainsAw/MothEffect/blob/main/Docs/05_Development_Guide/Development_Plan.md
https://github.com/YYchainsAw/MothEffect/blob/main/Docs/06_Test_Doc/Test_Plan.md
-->
