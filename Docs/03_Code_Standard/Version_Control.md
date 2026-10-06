# Moth Effect（飞蛾效应）：版本管理与 Git 排除规范

版本 v0.8 · 2026-10-06 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件定义仓库、排除规则与恢复约定。origin 为 https://github.com/YYchainsAw/MothEffect.git，远程为已公开、有提交的仓库。2026-10-06 核对 `main` 与 `develop` 均包含 [PR #30](https://github.com/YYchainsAw/MothEffect/pull/30) 的轻量 Actions 和 [PR #31](https://github.com/YYchainsAw/MothEffect/pull/31) 的 Issue/PR 模板；核对时两分支指向 `bea7273cf4a7319f85469eb572a10425c4e67ca6`。Git/origin/忽略/LFS 已配置，远程写入和 PR 合并已完成；里程碑 tag 仍以实际验收为依据。

## 1. 提交与忽略范围

Git 提交工程 `.uproject`、`Source/`、`Config/`、`Content/MothEffect/` 及本文档、[玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)、资源来源记录。Content 中其余目录与顶层文件全部忽略；这是用户指定的资源范围，仅影响版本管理，不删除本机资源。保留的 `.uasset/.umap` 用 Git LFS。忽略 `Binaries/Intermediate/Saved/DerivedDataCache/.vs`，不忽略整个 Plugins。每天可启动版本做一次提交，里程碑打 tag，改动规则时同步 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)/[设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md)/[玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)。

Docs 内 Markdown/JSON 是维护源文件，Reading_View.html 是由源文件生成的阅读快照；更新快照时与对应源文件同批提交。有效排除规则见根 [.gitignore](../../.gitignore)，有效 LFS/文本规则见根 [.gitattributes](../../.gitattributes)。首次提交时应一并纳入这两个文件；规则已配置，本次没有暂存资源或执行 LFS 上传。[GitHub：配置 Git LFS](https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage)

## 2. 提交与恢复

每天保存能启动的版本及构建标识，G0–G4 里程碑以实际验收为依据打 tag。规则、参数或范围改动按 [文档维护与变更流程](../05_Development_Guide/Documentation_Workflow.md) 同步，并在 [变更日志](../05_Development_Guide/Changelog.md) 留痕。

T03 的 [Issue #5](https://github.com/YYchainsAw/MothEffect/issues/5) 已关闭，Project 状态为 Done；版本与资源依赖恢复的验收项、恢复文件和证据字段仍待补。远程提交可核对不等于已验证完整工程恢复：需记录 Content/MothEffect 之外的实际依赖及补齐方法，从最近备份取回测试文件并核对内容。任务状态与工时见 [开发计划](../05_Development_Guide/Development_Plan.md)。

## 3. 资源来源

外部资源、授权与 AI 使用据实记录，资源验收见 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，投稿说明见 [发布检查清单](../07_Release/Release_Checklist.md)。不要把缓存目录当成项目源文件或唯一恢复来源。

## 4. .gitignore 的实际排除范围

规则只在根 .gitignore 维护，本说明用于解释，不作为第二套配置。

| 分类 | 排除内容 | 原因 |
|---|---|---|
| UE 生成目录 | 根 Binaries、Intermediate、Saved、DerivedDataCache | 编译结果、缓存、自动保存及日志，由本机重新生成 |
| 游戏资源范围 | Content 中 MothEffect 以外的全部顶层目录与文件 | 用户指定只保留 Content/MothEffect；其他资源留在本机 |
| 插件缓存 | Plugins 下的 Intermediate、Saved、DerivedDataCache | 可重新生成；插件 Binaries 默认保留，以兼容仅提供二进制的插件 |
| IDE 本地状态 | .vs、.idea、.vscode、.fleet、生成的根 .sln/.slnx/.vcxproj、用户设置 | 避免机器状态进入仓库；根 .vsconfig 保留 |
| 打包输出 | 根 Builds、Packages、Releases、Dist、Artifacts | 在发布渠道或单独保存构建包；不在这些目录存源文件 |
| 私人资料 | 根 _Local、Docs/Private、Config/Local、*.local.ini | 提供本机笔记和配置的明确位置，当前未搬移现有内容 |
| 凭证及临时文件 | .env、证书/签名容器及常见临时文件 | .env.example/.env.sample 保留，但其内容不能含真实凭证 |

保留 .uproject、Source、Config、Content/MothEffect、可公开的 Docs、Build 中图标/构建配置、SourceArt 及必要插件文件。不要全局排除图片、.obj 模型、DLL、LIB 或整个 Plugins；新增有源码插件时，确认其 Binaries 可重建后再添加精确排除规则。

早期扫描的 Content 共 753 个 .uasset/.umap、约 134.4 MiB；此统计覆盖全部本机资源，不代表当前提交范围。当前只保留 Content/MothEffect，其余 Content 和生成目录均排除，实际候选文件数量以 git ls-files --others --exclude-standard 为准。

当前 DefaultEngine.ini 的默认地图和游戏模式仍指向 /Game/ThirdPerson，源码/模板还使用 Characters、Input 等资源。按本轮规则，这些资源不会进入 Git；从远程全新克隆时，需要另行补齐模板资源或迁移依赖才能复现本机工程。本次未移动资产、修改引用或验证新克隆启动。以后若使用外置 Actor/Object 数据，还需核对 Content/__ExternalActors__、Content/__ExternalObjects__ 中是否存在本作关卡的必要数据；当前它们也按用户要求忽略。

.gitignore 只影响未跟踪文件，不能清除已有历史中的内容；未来误提交文件时需要另行处理索引或历史。[GitHub：忽略文件](https://docs.github.com/en/get-started/git-basics/ignoring-files)

## 5. Docs 公开建议与本次检查

现有 Docs 主要是本作策划、规则、技术方案、排期、测试和发布计划，适合在愿意公开完整设计的前提下发布。本轮已将个人磁盘路径替换为相对链接/路径，并同步 HTML 阅读快照；本次扫描未发现明显 Token、私钥或个人身份资料。Markdown 链接以所在文件目录为基准；JSON 的 projectRoot/documentAuthority 以该 JSON 所在目录为基准，pathBase 已明确声明。

| 内容类别 | 涉及文件 | 当前处理/约定 |
|---|---|---|
| 本地工程路径 | README、Project_Overview、Game_Design、Technical_Design、Design_Decisions、Development_Plan | 已保留工程与模块名称，并改为指向 .uproject 的相对链接 |
| 环境绑定字段 | Gameplay_Parameters.json 的 projectRoot/documentAuthority | 已改为 ../../.. 和 ../..，基于 JSON 所在目录；玩法数值未改 |
| 汇总副本 | Reading_View.html | 已按当前 Markdown/JSON 重新生成，移除旧个人路径 |
| 后续私人材料 | 后台截图、认证资料、真实试玩者信息及账户信息 | 保留在本地；Docs/Private 已忽略，阅读快照也不能汇入该目录 |

未实现规则和未执行测试继续保留对应状态。公开仓库允许他人查看及 fork，公开本身不自动授予 MIT 等开源许可。是否授权复用自有代码/文档由项目所有者决定；本次没有代选或新增 LICENSE。[GitHub：仓库许可](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository)

## 6. 首次公开提交前的项目检查

本项目当前目标为 Windows 单机，已在 Config/DefaultEngine.ini 的 AndroidFileServer 配置中设置 bEnablePlugin=False、bAllowNetworkConnection=False、SecurityToken=（显式空值），保留 Shipping、Shipping 外部启动和独立 AFS 编译开关为 False。清空 Token 同时关闭该服务器用途，避免空 Token 取消认证检查；本次未输出旧值。配置节之外的内容未修改，未启动编辑器、编译或打包。以后需要 Android 文件服务器时，另行配置新的私有 Token，不把实际凭证写入公开默认配置。[Epic：Android File Server](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-file-server-for-unreal-engine)

公开资源前逐项记录来源及可再分发范围。UE EULA 对 Samples/Templates 中的 Examples 允许源格式分发，不能把此权限推广到所有引擎内容或商城资产。Fab 标准许可允许项目使用及向项目协作者共享资产，同时限制素材单独再分发；公开可提取的原始资源需要依据其实际许可判断。[Unreal Engine EULA](https://www.unrealengine.com/en-US/eula/unreal)、[Fab 标准许可](https://www.fab.com/eula)

## 7. GitHub 分支、PR 与检查

日常任务、分支、Project Status、模板及 Actions 用法见 [GitHub 开发流程](../05_Development_Guide/GitHub_Workflow.md)。PR 通过 `Refs #实际编号` 关联任务；游戏改动合并后补齐受影响的 UE 验收再完成任务。静态 CI 检查 JSON、文档相对文件链接和 LFS 属性，不验证资源上传完整性、编译、打包或恢复。
