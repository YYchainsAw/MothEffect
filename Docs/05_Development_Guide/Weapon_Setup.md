# Moth Effect（飞蛾效应）：步枪、换弹与生命接入

版本 v0.2 · 2026-10-03 · T05 进行中 · C++ 已写入，编译与运行待用户验收

用户报告上一轮移动/瞄准动画测试成功，本轮推进步枪闭环。协作方式继续为助手修改 C++、用户配置 UE 资产与试玩；助手没有编译、运行 PIE 或修改二进制资产。新增原生类/接口/反射字段需由用户保存工作、关闭编辑器后自行完成 C++ 编译，再打开工程配置。

本轮先实现 W01、Ready/Reloading/Dead、生命组件和命中接口。Carrying/ThrowRecovery/Throw 已保留枚举，拾取、投掷、机关、AI、HUD 和整局胜负随后续任务接入；T05 不因源码存在而标记完成。行为契约见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)，武器与生命数值按 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)。

## 1. 已写入的 C++

| 文件 | 内容 |
|---|---|
| [Rifle.h](../../Source/MothEffect/Public/Weapons/Rifle.h) / [Rifle.cpp](../../Source/MothEffect/Private/Weapons/Rifle.cpp) | 枪网格/枪口组件；射速、弹匣、手动换弹；相机到目标与枪口到目标检测；枪口短段/重叠检查；开火、空匣、受阻、弹药与换弹事件 |
| [HealthComponent.h](../../Source/MothEffect/Public/Components/HealthComponent.h) / [HealthComponent.cpp](../../Source/MothEffect/Private/Components/HealthComponent.cpp) | 血量、HitId 去重、一次死亡；暂停/结束/死亡后拒绝伤害 |
| [HitContext.h](../../Source/MothEffect/Public/Types/HitContext.h) / [BallisticReactive.h](../../Source/MothEffect/Public/Interfaces/BallisticReactive.h) | 蓝图可用 ST_HitContext 与 Receive Ballistic Hit；角色/测试靶/后续机关共用命中入口 |
| [PlayerActionState.h](../../Source/MothEffect/Public/Types/PlayerActionState.h) | 玩家行动和左键归属枚举 |
| [MothEffectCharacter.h](../../Source/MothEffect/Public/Characters/MothEffectCharacter.h) / [MothEffectCharacter.cpp](../../Source/MothEffect/Private/Characters/MothEffectCharacter.cpp) | 输入授权、生成并挂接步枪、行动转换、开火/换弹 Montage、死亡停止玩法 |
| [MothEffectPlayerController.cpp](../../Source/MothEffect/Private/Framework/MothEffectPlayerController.cpp) | 输入 Flush/暂停时停火，要求实际松开左键后重新按；暂停保留换弹任务 |

步枪用 Timer 连射，松键不会清除射速冷却，因此快速点击不能绕过射速。枪口受阻仍消耗弹药，但不调用伤害接口。空匣不自动换弹；满匣按 R 不启动任务。换弹取消不补弹，只有当前有效任务到期才补满；无限备弹，不另存一份备用弹药数。动画只负责表现，不添加 Notify 来扣弹、判定命中或补弹。[Epic：Gameplay Timers](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-timers-in-unreal-engine)

本轮左键松键检查按 Windows 桌面鼠标配置实现；后续增加手柄/重绑定时再扩展物理按键查询。完整暂停菜单和局内状态框架尚未制作。

## 2. 先接入开火与换弹输入

在 `/Game/MothEffect/Input/Actions` 创建以下动作，Value Type 都为 Digital/Bool，Triggers、Modifiers 留空：

| 动作 | IMC_MothGameplay 的按键 | BP_ThirdPersonCharacter 的 Input 引用 |
|---|---|---|
| IA_Primary | Left Mouse Button | Primary Action |
| IA_Reload | R | Reload Action |

把映射追加到现有 `/Game/MothEffect/Input/Mapping/IMC_MothGameplay`。控制器此前已追加该上下文，不再重复添加，也不移除原有 Move/Look/Jump/Aim/Sprint。角色 C++ 绑定 Primary 的 Started/Completed/Canceled 与 Reload 的 Started，蓝图不要再写第二套输入开火。

## 3. 创建并挂接 BP_Rifle

1. 用户已在 `/Game/MothEffect/Weapons` 创建 **BP_Rifle**，沿用该资产；确认其父类为 **Rifle**。
2. 打开 Components，选继承的 **WeaponMesh**，指定枪械 Static Mesh；没有枪模型时可先用缩放后的长方体。枪体 Collision 为 NoCollision，由 C++ 射线完成命中。
3. 调整 WeaponMesh 的相对位置/旋转；选 **Muzzle**，把它放到实际枪管前端。Muzzle 是 Scene Component，不需要添加碰撞球。先让枪械蓝图的 +X 朝枪口前方，避免网格与枪口各朝不同方向。
4. 在 Class Defaults → Weapon 核对参数与账本；测试时勾选 Weapon → Debug → **Draw Debug Shots**。
5. 用户已在 `hand_r` 下创建 **WeaponSocket**。保存该 Socket 所属骨架/网格资产；打开现有 `BP_ThirdPersonCharacter`，Class Defaults → Player → Weapon：**Rifle Class=BP_Rifle**，**Rifle Attach Socket=WeaponSocket**。C++ 默认值已同步为 WeaponSocket；已有蓝图可能仍保存旧值，需要在编辑器明确核对。
6. 若使用 WeaponSocket 调整持枪位置，先将 **Rifle Relative Transform** 设为位置/旋转零、缩放一，在 Socket 上完成对齐；需要额外偏移时再修改此字段。先用骨架 Socket 的 Preview Asset 辅助对齐，再在 PIE 看实际挂接；Preview Asset 不会生成运行时武器。

玩家 BeginPlay 自动生成一把 Owned Rifle 并挂到 Mesh，不在关卡另摆第二把枪，也不在角色 Construction Script 重复生成。角色退出时清除其枪械和定时器。当前 ARifle 不需要 Event Tick 或额外 Blueprint 开火计时器。

当前 **Weapon Trace Channel 默认 Visibility**，用于先打通验证房。墙和靶子至少有一层碰撞 Block Visibility；不要让场景中用于提示/触发的透明碰撞体挡住它。正式 `JamWeaponTrace` 的配置与矩阵仍按 TDD 接入，未配置前不要把这一阶段当作正式碰撞验收完成。

## 4. 创建一个可扣血的射击测试靶

1. 在 `/Game/MothEffect/Tests` 创建 Actor 蓝图 **BP_ShootingTarget**。添加一个有碰撞的 Static Mesh（Cube 即可），设 Block Visibility，放在玩家面前。
2. Add Component → 添加本项目的 **Health Component**。默认 Max Health 使用 player.maxHealth；需要其他目标血量时在组件默认值中指定。
3. Class Settings → Implemented Interfaces，添加 **Ballistic Reactive**。接口允许蓝图实现，不需要创建敌人 C++ 类。[Epic：Interfaces](https://dev.epicgames.com/documentation/en-us/unreal-engine/interfaces-in-unreal-engine)
4. 在 My Blueprint → Interfaces 打开 **Receive Ballistic Hit**。它有 bool 返回值，通常显示为函数图。把输入 **Context** 接到 Health Component 的 **Apply Hit**，执行链调用后将返回值接到函数 Return Node。

~~~text
Receive Ballistic Hit(Context)
  → Health Component.Apply Hit(Context)
  → Return（Apply Hit 的 bool 返回值）
~~~

5. 选 Health Component，在 Details → Events 添加 **On Health Changed**，Print String 显示 Current Health；添加 **On Died**，打印一次“Target Dead”，再隐藏靶子并关闭 Actor Collision。先保留 Actor，方便查看一次死亡和当前血量。

靶子只转发已有命中上下文，不自己构造新的 HitId、叠加伤害或修改弹匣。步枪给每发命中创建唯一 HitId，Health Component 负责去重；后续敌人和机关可沿用这个入口。

到这里即可先验收实际射击，不要求 Montage 已配置。准星临时放在屏幕中心；没有 UI 时根据青色相机检测线定位目标。青色是相机段、黄色是胸前到枪口的安全段，红色是枪口段，受阻时枪口段显示橙色。

## 5. 再接入上半身射击与换弹

在 `/Game/MothEffect/Animations/Player/Combat` 创建：

| Montage | 先引用的模板动画 | 角色 Player → Animation 字段 |
|---|---|---|
| AM_MothRifleFire | /Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire | Fire Montage |
| AM_MothRifleReload | /Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload | Reload Montage |

先核对与玩家 Mesh 的骨架兼容和动画预览；不把 AO 的 Additive 样本当普通射击/换弹序列。关闭推动玩家的 Root Motion。射击每次实际扣一发由 C++ 请求 Montage；换弹只在有效任务开始时播放，C++ 根据 Montage 长度调整播放速率以匹配换弹时长。

在 Anim Slot Manager 创建 **UpperBody** Slot（可放在 DefaultGroup），两个 Montage 的 Slot 都改为 **DefaultGroup.UpperBody**。同组同时只保留一个 Montage，有效换弹会接替正在播放的射击；动作是否允许由 C++ 决定。[Epic：Animation Slots](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-slots-in-unreal-engine)

打开 ABP_MothPlayer，保留已经验证的移动状态机与 AimOffset 混合。在 **Two Way Blend 的输出之后**缓存持枪基础姿态，再分两路：

~~~text
移动状态机 + AO_Rifle + Two Way Blend → Save Cached Pose（MothArmedBase）
Use Cached Pose（MothArmedBase）───────────────────→ Layered Blend per Bone.Base Pose
Use Cached Pose（MothArmedBase）→ Slot（UpperBody）→ Layered Blend per Bone.Blend Pose 0
Layered Blend per Bone → 原有 FootIK / Control Rig → Output Pose
~~~

Layered Blend per Bone 先用 Branch Filter；Blend Pose 0 的权重为 1，在 Layer Setup[0] 添加 Bone Name=`spine_01`、Blend Depth=1。以玩家实际 Skeleton 的脊柱骨名为准。这样上半身播放 Montage，腿部继续使用移动/腾空姿态；仅添加 Slot 而不做骨骼分层会让换弹覆盖全身。替换原先输出链上的全身 Slot，避免同一个 Montage 经两条链重复覆盖。若现有 IK 覆盖手臂结果，先检查其骨骼作用范围。

Reloading 时若现有 AimOffset 扭曲换弹姿势，可在 Event Graph 读取 **Get Action State**，将 Reloading 的动画 AO 权重降低或设 0；这只调整动画，不取消玩家右键瞄准和镜头控制。

## 6. 本轮验收顺序

记录 PIE 构建/实际参数与结果，对应 TC24、TC34–TC36、TC38 的武器分支；不要一次勾选整条案例通过。

1. 点击/长按左键能单发/连射，松键停火；移动、瞄准、跳跃中仍能命中。快速点击不能突破射速。
2. 看 Get Rifle → Get Ammo In Magazine；打空后不再命中、不自动换弹。少弹时 R 开始任务，完成后补满；满匣 R 没有动作。
3. 按住左键再按 R，或者换弹期间按住左键；换弹结束都不自动开火，松开后重新按才开火。普通受伤不取消换弹。
4. 枪口靠墙/进入墙体时仍扣弹，但墙后靶子不受伤；相机能看到靶子也不能穿墙射击。调试线和实际血量一起检查。
5. 固定伤害命中后靶子逐次扣血；血量到零只广播一次死亡，后续不再接受伤害。同一 HitId 重放给 Apply Hit 也不能重复扣血。
6. 冲刺中有效开火立即取消冲刺，仍按住 Shift 不自动恢复；需松开再按 Shift。射击/换弹动画不锁腿部，不影响转向/跳跃。
7. 连射时 Alt+Tab 后停火；返回且仍按住左键不会续射，实际松开再按恢复。R 换弹期间停止 Montage，计时仍正常完成，不留下 Reloading。
8. 通过控制器 Set Pause 测暂停：停止连射、清瞄准/冲刺，换弹进度冻结；恢复继续剩余计时，左键仍须重新按。当前没有暂停菜单，可先用测试蓝图按键调用。
9. 换弹期间调用角色 Set Gameplay Enabled(false) 或让角色死亡：不补弹、不继续攻击；EndPlay/重新进入 PIE 不残留任务。暂停请用 Set Pause，Set Gameplay Enabled(false) 是结算取消语义。

这轮通过后进入 T06/T07：装置基类、拾取/放下/安全投掷，再接 D03 空中射击启动。先验证一个灰盒装置的完整操作，不提前批量制作机关效果。
