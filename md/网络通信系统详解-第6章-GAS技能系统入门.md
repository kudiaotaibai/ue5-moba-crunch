# Crunch 网络通信系统详解 - 第6章：GAS 技能系统入门

> 这一章我们来学习 Gameplay Ability System（GAS），这是 UE5 官方提供的强大技能框架。

---

## 目录

1. [GAS 是什么](#gas-是什么)
2. [核心概念解释](#核心概念解释)
3. [CAbilitySystemComponent 详解](#cabilitysystemcomponent-详解)
4. [技能初始化流程](#技能初始化流程)
5. [属性系统详解](#属性系统详解)

---

## GAS 是什么

### 通俗解释

**GAS = Gameplay Ability System（游戏玩法能力系统）**

```
GAS 就像一个"技能管理器"
它帮你管理：
- 角色属性（生命值、法力值、攻击力等）
- 技能释放（技能冷却、消耗、效果）
- 状态效果（眩晕、加速、护盾等）
```

### 为什么要用 GAS？

**❌ 不用 GAS 的问题：**
```cpp
// 每个技能都要自己写一堆代码
void CastFireball()
{
    // 检查法力值
    if (Mana < 50) return;
    
    // 检查冷却时间
    if (FireballCooldown > 0) return;
    
    // 扣除法力值
    Mana -= 50;
    
    // 设置冷却
    FireballCooldown = 5.0f;
    
    // 播放动画
    PlayAnimation(...);
    
    // 造成伤害
    DealDamage(...);
    
    // 网络同步
    Server_CastFireball();
}

// 每个技能都要重复这些逻辑！
```

**✅ 使用 GAS 的好处：**
```cpp
// GAS 帮你处理了所有通用逻辑
void CastFireball()
{
    // 一行代码搞定！
    AbilitySystemComponent->TryActivateAbilityByClass(FireballAbilityClass);
    
    // GAS 自动处理：
    // ✅ 检查法力值
    // ✅ 检查冷却时间
    // ✅ 扣除法力值
    // ✅ 设置冷却
    // ✅ 网络同步
}
```

---

## 核心概念解释

### 1. Ability System Component（ASC）

**通俗解释：**
```
ASC = 技能系统组件
就像角色的"技能背包"
里面装着所有技能和属性
```

**作用：**
- 管理角色的所有技能
- 管理角色的所有属性
- 处理技能的激活和取消
- 处理属性的变化和同步

### 2. Gameplay Ability（GA）

**通俗解释：**
```
GA = 游戏玩法能力
就是一个"技能"
比如：火球术、治疗术、冲刺
```

**一个技能包含：**
- 消耗（Cost）：需要多少法力值
- 冷却（Cooldown）：多久能再次使用
- 效果（Effect）：造成伤害、治疗、加速等
- 动画（Animation）：播放什么动画

### 3. Gameplay Effect（GE）

**通俗解释：**
```
GE = 游戏玩法效果
就是"技能的效果"
比如：造成100点伤害、恢复50点生命、增加20%移速
```

**效果类型：**
- **Instant（瞬时）：** 立即生效，比如造成伤害
- **Duration（持续）：** 持续一段时间，比如5秒加速
- **Infinite（无限）：** 永久生效，比如装备属性加成

### 4. Attribute Set（属性集）

**通俗解释：**
```
Attribute Set = 属性集合
就是角色的"属性面板"
比如：生命值、法力值、攻击力、护甲
```

**属性特点：**
- 自动网络同步
- 可以被 Gameplay Effect 修改
- 可以监听变化事件

### 5. Gameplay Tag（游戏标签）

**通俗解释：**
```
Gameplay Tag = 游戏标签
就像给技能和状态贴"标签"
比如：Ability.Attack.Melee（近战攻击技能）
      State.Stunned（眩晕状态）
```

**用途：**
- 标识技能类型
- 标识角色状态
- 控制技能能否释放（眩晕时不能释放技能）

---

## CAbilitySystemComponent 详解

### 类的继承关系

```
UAbilitySystemComponent（UE5 官方基类）
    ↓
UCAbilitySystemComponent（我们的自定义类）
```

**为什么要继承？**
- 添加项目特定的功能
- 自定义初始化逻辑
- 添加辅助函数

### 代码详解：构造函数

**文件位置：** `Source/Crunch/Private/GAS/CAbilitySystemComponent.cpp:14`

```cpp
UCAbilitySystemComponent::UCAbilitySystemComponent()
{
    // 1. 监听生命值变化
    GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute())
        .AddUObject(this, &UCAbilitySystemComponent::HealthUpdated);
    
    // 2. 监听法力值变化
    GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetManaAttribute())
        .AddUObject(this, &UCAbilitySystemComponent::ManaUpdated);
    
    // 3. 监听经验值变化
    GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetExperienceAttribute())
        .AddUObject(this, &UCAbilitySystemComponent::ExperienceUpdated);
    
    // 4. 设置确认和取消输入ID
    
    GenericConfirmInputID = (int32)ECAbilityInputID::Confirm;
    GenericCancelInputID  = (int32)ECAbilityInputID::Cancel;
    
}
```

**逐段解释：**

#### 第1步：监听属性变化

```cpp
GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute())
    .AddUObject(this, &UCAbilitySystemComponent::HealthUpdated);
```

- **GetGameplayAttributeValueChangeDelegate()：** 获取属性变化委托
- **UCAttributeSet::GetHealthAttribute()：** 获取生命值属性
- **AddUObject()：** 添加回调函数
- **HealthUpdated：** 生命值变化时调用的函数

**通俗解释：**
```
告诉 GAS："生命值变化时，请调用我的 HealthUpdated() 函数"
就像订阅通知："生命值有变化时通知我"
```

**为什么要监听？**
```
生命值 <= 0 → 角色死亡
生命值 >= 最大值 → 满血状态
```

#### 第2步：设置输入ID

```cpp
GenericConfirmInputID = (int32)ECAbilityInputID::Confirm;
GenericCancelInputID = (int32)ECAbilityInputID::Cancel;
```

- **GenericConfirmInputID：** 确认输入ID
- **GenericCancelInputID：** 取消输入ID
- **ECAbilityInputID：** 输入ID枚举

**用途：**
```
某些技能需要"确认"或"取消"
比如：
1. 按Q键 → 进入瞄准模式
2. 按鼠标左键 → 确认释放
3. 按鼠标右键 → 取消释放
```

---

## 技能初始化流程

### 完整流程图

```
服务器启动
    ↓
角色生成（Spawn）
    ↓
BeginPlay()
    ↓
调用 ServerSideInit()
    ↓
┌─────────────────────────────┐
│ 第1步：InitializeBaseAttribute() │
│ 初始化基础属性                │
│ - 生命值                      │
│ - 法力值                      │
│ - 攻击力                      │
│ - 护甲                        │
│ - 移速                        │
│ - 力量、智力                  │
└─────────────┬───────────────┘
              ↓
┌─────────────────────────────┐
│ 第2步：ApplyInitialEffects()  │
│ 应用初始效果                  │
│ - 满血满蓝                    │
│ - 初始属性加成                │
└─────────────┬───────────────┘
              ↓
┌─────────────────────────────┐
│ 第3步：GiveInitalAbilities()  │
│ 赋予初始技能                  │
│ - 主动技能（Q、W、E、R）      │
│ - 基础技能（普攻、移动）      │
│ - 被动技能                    │
└─────────────────────────────┘
    ↓
初始化完成，角色可以使用技能
```

### 代码详解：ServerSideInit()

```cpp
void UCAbilitySystemComponent::ServerSideInit()
{
    InitializeBaseAttribute();  // 初始化基础属性
    ApplyInitialEffects();      // 应用初始效果
    GiveInitalAbilities();      // 赋予初始技能
}
```

**为什么叫 ServerSideInit？**
```
因为这些操作只在服务器执行
客户端通过网络同步获得结果
```

---

## 属性系统详解

### 代码详解：InitializeBaseAttribute()

**文件位置：** `Source/Crunch/Private/GAS/CAbilitySystemComponent.cpp:23`

```cpp
void UCAbilitySystemComponent::InitializeBaseAttribute()
{
    // 1. 检查数据是否有效
    if (!AbilitySystemGenerics || 
        !AbilitySystemGenerics->GetBaseStatDataTable() || 
        !GetOwner())
    {
        return;
    }

    // 2. 获取基础属性数据表
    const UDataTable* BaseStatDataTable = AbilitySystemGenerics->GetBaseStatDataTable();
    const FHeroBaseStats* BaseStats = nullptr;

    // 3. 查找当前角色的基础属性
    for (const TPair<FName, uint8*>& DataPair : BaseStatDataTable->GetRowMap())
    {
        BaseStats = BaseStatDataTable->FindRow<FHeroBaseStats>(DataPair.Key, "");
        if (BaseStats && BaseStats->Class == GetOwner()->GetClass())
        {
            break;  // 找到了，跳出循环
        }
    }
    
    // 4. 设置基础属性
    if (BaseStats)
    {
        // 基础战斗属性
        SetNumericAttributeBase(UCAttributeSet::GetMaxHealthAttribute(), BaseStats->BaseMaxHealth);
        SetNumericAttributeBase(UCAttributeSet::GetMaxManaAttribute(), BaseStats->BaseMaxMana);
        SetNumericAttributeBase(UCAttributeSet::GetAttackDamageAttribute(), BaseStats->BaseAttackDamage);
        SetNumericAttributeBase(UCAttributeSet::GetArmorAttribute(), BaseStats->BaseArmor);
        SetNumericAttributeBase(UCAttributeSet::GetMoveSpeedAttribute(), BaseStats->BaseMoveSpeed);

        // 英雄属性
        SetNumericAttributeBase(UCHeroAttributeSet::GetStrengthAttribute(), BaseStats->Strength);
        SetNumericAttributeBase(UCHeroAttributeSet::GetStrengthGrowthRateAttribute(), BaseStats->StrengthGrowthRate);
        SetNumericAttributeBase(UCHeroAttributeSet::GetIntelligenceAttribute(), BaseStats->Intelligence);
        SetNumericAttributeBase(UCHeroAttributeSet::GetIntelligenceGrowthRateAttribute(), BaseStats->IntelligenceGrowthRate);
    }

    // 5. 设置等级相关属性
    const FRealCurve* ExperienceCurve = AbilitySystemGenerics->GetExperienceCurve();
    if (ExperienceCurve)
    {
        // 获取最大等级
        int MaxLevel = ExperienceCurve->GetNumKeys();
        SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelAttribute(), MaxLevel);

        // 获取满级所需经验
        float MaxExp = ExperienceCurve->GetKeyValue(ExperienceCurve->GetLastKeyHandle());
        SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelExperienceAttribute(), MaxExp);

        UE_LOG(LogTemp, Warning, TEXT("MaxLevel:%d, Max exp:%f"), MaxLevel, MaxExp);
    }

    // 6. 触发经验值更新（初始化等级）
    ExperienceUpdated(FOnAttributeChangeData());
}
```

**逐段详解：**

#### 第1步：检查数据有效性

```cpp
if (!AbilitySystemGenerics || 
    !AbilitySystemGenerics->GetBaseStatDataTable() || 
    !GetOwner())
{
    return;
}
```

- **AbilitySystemGenerics：** 技能系统通用数据资产
- **GetBaseStatDataTable()：** 获取基础属性数据表
- **GetOwner()：** 获取拥有这个组件的 Actor（角色）

**通俗解释：**
```
检查"配置文件"是否存在
如果没有配置，就没法初始化属性
```

#### 第2步：查找角色的基础属性

```cpp
for (const TPair<FName, uint8*>& DataPair : BaseStatDataTable->GetRowMap())
{
    BaseStats = BaseStatDataTable->FindRow<FHeroBaseStats>(DataPair.Key, "");
    if (BaseStats && BaseStats->Class == GetOwner()->GetClass())
    {
        break;
    }
}
```

- **GetRowMap()：** 获取数据表的所有行
- **FindRow<FHeroBaseStats>()：** 查找一行数据
- **BaseStats->Class：** 这行数据对应的角色类
- **GetOwner()->GetClass()：** 当前角色的类

**通俗解释：**
```
数据表就像一个 Excel 表格：

| 角色类      | 生命值 | 法力值 | 攻击力 |
|------------|--------|--------|--------|
| Crunch     | 600    | 300    | 50     |
| Phase      | 500    | 400    | 40     |

这段代码就是在表格里找到当前角色对应的那一行
```

#### 第3步：设置基础属性

```cpp
SetNumericAttributeBase(UCAttributeSet::GetMaxHealthAttribute(), BaseStats->BaseMaxHealth);
```

- **SetNumericAttributeBase()：** 设置属性的基础值
- **GetMaxHealthAttribute()：** 获取最大生命值属性
- **BaseStats->BaseMaxHealth：** 从数据表读取的值

**Base Value vs Current Value：**
```
Base Value（基础值）：
- 角色的原始属性
- 不受临时效果影响
- 比如：基础生命值 = 600

Current Value（当前值）：
- 实际的属性值
- 受装备、Buff 影响
- 比如：当前生命值 = 600 + 100（装备加成） = 700
```

**属性列表：**

| 属性 | 说明 | 举例 |
|------|------|------|
| MaxHealth | 最大生命值 | 600 |
| MaxMana | 最大法力值 | 300 |
| AttackDamage | 攻击力 | 50 |
| Armor | 护甲 | 20 |
| MoveSpeed | 移动速度 | 500 |
| Strength | 力量 | 20 |
| StrengthGrowthRate | 力量成长率 | 2.5/级 |
| Intelligence | 智力 | 15 |
| IntelligenceGrowthRate | 智力成长率 | 2.0/级 |

#### 第4步：设置等级系统

```cpp
const FRealCurve* ExperienceCurve = AbilitySystemGenerics->GetExperienceCurve();
if (ExperienceCurve)
{
    int MaxLevel = ExperienceCurve->GetNumKeys();
    SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelAttribute(), MaxLevel);

    float MaxExp = ExperienceCurve->GetKeyValue(ExperienceCurve->GetLastKeyHandle());
    SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelExperienceAttribute(), MaxExp);
}
```

- **ExperienceCurve：** 经验曲线（升级所需经验）
- **GetNumKeys()：** 获取曲线的关键点数量（等级数）
- **GetLastKeyHandle()：** 获取最后一个关键点
- **GetKeyValue()：** 获取关键点的值（满级所需经验）

**经验曲线示例：**

```
等级 | 所需经验
-----|----------
1    | 0
2    | 100
3    | 250
4    | 450
5    | 700
...  | ...
20   | 10000
```

**通俗解释：**
```
ExperienceCurve 就像一个"升级表"
告诉系统：
- 最高等级是多少（20级）
- 满级需要多少经验（10000点）
```

---

## 应用初始效果

### 代码详解：ApplyInitialEffects()

```cpp
void UCAbilitySystemComponent::ApplyInitialEffects()
{
    // 1. 检查权限
    if (!GetOwner() || !GetOwner()->HasAuthority())
        return;


    // 2. 检查数据
    if (!AbilitySystemGenerics)
        return;


    // 3. 应用所有初始效果
    for (const TSubclassOf<UGameplayEffect>& EffectClass : 
         AbilitySystemGenerics->GetInitialEffects())
    {
        // 创建效果规格
        FGameplayEffectSpecHandle EffectSpecHandle = 
            MakeOutgoingSpec(EffectClass, 1, MakeEffectContext());
        
        // 应用到自己
        ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
    }
    
}
```

**关键概念：**

#### Gameplay Effect Spec（效果规格）

```cpp
FGameplayEffectSpecHandle EffectSpecHandle = 
    MakeOutgoingSpec(EffectClass, 1, MakeEffectContext());
```

- **EffectClass：** 效果类（蓝图或C++类）
- **Level：** 效果等级（1级）
- **EffectContext：** 效果上下文（谁释放的、在哪释放的）

**通俗解释：**
```
Gameplay Effect Class = 效果模板
就像一个"配方"，定义了效果的规则

Gameplay Effect Spec = 效果实例
就像根据"配方"做出来的"成品"
包含了具体的数值和上下文信息
```

**初始效果通常包括：**
- 满血满蓝效果（Health = MaxHealth, Mana = MaxMana）
- 初始金币（Gold = 500）
- 初始等级（Level = 1）

---

## 赋予初始技能

### 代码详解：GiveInitalAbilities()

```cpp
void UCAbilitySystemComponent::GiveInitalAbilities()
{
    // 1. 检查权限
    if (!GetOwner() || !GetOwner()->HasAuthority())
        return;
    
    // 2. 赋予主动技能（Q、W、E、R）
    for (const TPair<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : Abilities)
    {
        GiveAbility(FGameplayAbilitySpec(
            AbilityPair.Value,  // 技能类
            0,                  // 初始等级（0级，需要升级）
            (int32)AbilityPair.Key,  // 输入ID
            nullptr             // 源对象
        ));
    }

    // 3. 赋予基础技能（普攻、移动）
    for (const TPair<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : BasicAbilities)
    {
        GiveAbility(FGameplayAbilitySpec(
            AbilityPair.Value,  // 技能类
            1,                  // 初始等级（1级，直接可用）
            (int32)AbilityPair.Key,  // 输入ID
            nullptr
        ));
    }
    
    // 4. 赋予被动技能
    if (!AbilitySystemGenerics) return;
    for (const TSubclassOf<UGameplayAbility>& PassiveAbility : 
         AbilitySystemGenerics->GetPassiveAbilities())
    {
        GiveAbility(FGameplayAbilitySpec(
            PassiveAbility,  // 技能类
            1,               // 初始等级
            -1,              // 无输入ID（被动技能不需要按键）
            nullptr
        ));
    }
}
```

**技能类型：**

#### 1. 主动技能（Abilities）

```cpp
TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities;
```

- **初始等级：** 0级（需要升级才能使用）
- **输入ID：** Q、W、E、R
- **举例：**
  - Q：连击技能
  - W：地面冲击
  - E：射击
  - R：大招

#### 2. 基础技能（BasicAbilities）

```cpp
TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;
```

- **初始等级：** 1级（直接可用）
- **输入ID：** 鼠标左键、右键
- **举例：**
  - 鼠标左键：普通攻击
  - 鼠标右键：移动

#### 3. 被动技能（PassiveAbilities）

```cpp
TArray<TSubclassOf<UGameplayAbility>> PassiveAbilities;
```

- **初始等级：** 1级
- **输入ID：** -1（无输入，自动触发）
- **举例：**
  - 生命回复（每秒回复1%生命值）
  - 法力回复（每秒回复2%法力值）

---

## 常见问题

### Q1：为什么主动技能初始等级是0？

**A：** 这是 MOBA 游戏的设计：
```
游戏开始时：
- 角色1级
- 技能0级（不能用）
- 有1个技能点

玩家升级后：
- 获得技能点
- 用技能点升级技能
- 技能变成1级，可以使用了
```

### Q2：技能等级和角色等级有什么区别？

**A：**
```
角色等级：
- 角色的整体等级
- 升级获得技能点
- 影响基础属性

技能等级：
- 单个技能的等级
- 用技能点升级
- 影响技能威力
```

### Q3：为什么要在服务器初始化？

**A：** 防止作弊：
```
如果在客户端初始化：
- 玩家可以修改属性
- 玩家可以修改技能等级
- 玩家可以作弊

在服务器初始化：
- 服务器是权威
- 客户端只能接收
- 无法作弊
```

---

## 下一章预告

在第7章，我们会讲解：
- 🎯 技能释放的完整流程
- 💥 Gameplay Effect 的详细用法
- 🎮 技能升级系统

继续加油！
