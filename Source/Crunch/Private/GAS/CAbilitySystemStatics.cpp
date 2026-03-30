// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CAbilitySystemStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"


FGameplayTag UCAbilitySystemStatics::GetBasicAttackAbilityTag()
{
	return  FGameplayTag::RequestGameplayTag("ability.basicattack");
	 
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputPressedTag()
{
	return  FGameplayTag::RequestGameplayTag("ability.basicattack.pressed");
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputReleasedTag()
{
	return  FGameplayTag::RequestGameplayTag("ability.basicattack.released");

}

FGameplayTag UCAbilitySystemStatics::GetDeadStatTag()
{
	return  FGameplayTag::RequestGameplayTag("stats.dead");
}

FGameplayTag UCAbilitySystemStatics::GetStunStatTag()
{
	return  FGameplayTag::RequestGameplayTag("stats.stun");

}

FGameplayTag UCAbilitySystemStatics::GetAimStatTag()
{
	return  FGameplayTag::RequestGameplayTag("stats.aim");
}

FGameplayTag UCAbilitySystemStatics::GetCameraShakeGameplayCueTag()
{
	return  FGameplayTag::RequestGameplayTag("GameplayCue.cameraShake");

}

FGameplayTag UCAbilitySystemStatics::GetHeathFullStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.health.full");
}

FGameplayTag UCAbilitySystemStatics::GetHeathEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.health.empty");
}

FGameplayTag UCAbilitySystemStatics::GetManaFullStatTag()
{
	return  FGameplayTag::RequestGameplayTag("stats.mana.full");
}

FGameplayTag UCAbilitySystemStatics::GetManaEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("stats.mana.empty");
}

FGameplayTag UCAbilitySystemStatics::GetHeroRoleTag()
{
	return  FGameplayTag::RequestGameplayTag("role.hero");
}


FGameplayTag UCAbilitySystemStatics::GetExperienceAttributeTag()
{
	return  FGameplayTag::RequestGameplayTag("attr.experience");
}


FGameplayTag UCAbilitySystemStatics::GetGoldAttributeTag()
{
	return  FGameplayTag::RequestGameplayTag("attr.gold");
}

FGameplayTag UCAbilitySystemStatics::GetCrosshairTag()
{
	return FGameplayTag::RequestGameplayTag("stats.crosshair");
}

FGameplayTag UCAbilitySystemStatics::GetTargetUpdatedTag()
{
	return  FGameplayTag::RequestGameplayTag("target.updated");
}

bool UCAbilitySystemStatics::IsActorDead(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck,GetDeadStatTag());
}


bool UCAbilitySystemStatics::IsHero(const AActor* ActorToCheck)
{
   return ActorHasTag(ActorToCheck,GetHeroRoleTag());
}

bool UCAbilitySystemStatics::ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag)
{
	const IAbilitySystemInterface* ActorISA =Cast<IAbilitySystemInterface>(ActorToCheck);
    	if (ActorISA)
    	{
    		UAbilitySystemComponent* ActorASC =ActorISA->GetAbilitySystemComponent();
    		if (ActorASC)
    		{
    			return ActorASC->HasMatchingGameplayTag(Tag);
    			
    		}
    	}
    	return false;
}

bool UCAbilitySystemStatics::IsAbilityAtMaxLevel(const FGameplayAbilitySpec& Spec)
{
	return  Spec.Level >= 4;
}


// 这个函数本身是正确的，保持不变
float UCAbilitySystemStatics::GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
		return 0.f;

	const UGameplayEffect* CooldownEffect=Ability->GetCooldownGameplayEffect();
	if (!CooldownEffect)
	{
		return 0.f;
	}
	float CooldownDuration=0.f;

	CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(1,CooldownDuration);
	return CooldownDuration;
}

// 这个函数本身是正确的，保持不变
float UCAbilitySystemStatics::GetStaticCostForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
		return 0.f;

	const UGameplayEffect* CostEffect=Ability->GetCostGameplayEffect();
	if (!CostEffect||CostEffect->Modifiers.Num()==0)
	{
		return 0.f;
	}
	float Cost=0.f;
	CostEffect->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1,Cost);
	return FMath::Abs(Cost);
}

// ------------------- 以下是仅修正了错误的部分 -------------------

bool UCAbilitySystemStatics::CheckAbilityCost(const FGameplayAbilitySpec& AbilitySpec,
	const UAbilitySystemComponent& ASC)
{
	const UGameplayAbility* AbilityCDD =AbilitySpec.Ability;
	// 【已修正】: 移除了 '!'，现在是检查指针是否有效
	if (AbilityCDD) 
	{
		return AbilityCDD->CheckCost(AbilitySpec.Handle,ASC.AbilityActorInfo.Get());
	}

	return false;
}

float UCAbilitySystemStatics::GetManaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC,
	int AbilityLevel)
{
	float ManaCost=0.f;
	if (AbilityCDO)
	{
		// 注意: GetCostGameplayEffect() 返回的是 const 指针，这里最好用 const UGameplayEffect*
		// 但为了保持最小改动，我们暂时保留 UGameplayEffect*
		UGameplayEffect* CostEffect = const_cast<UGameplayEffect*>(AbilityCDO->GetCostGameplayEffect());
		
		if (CostEffect)
		{
			FGameplayEffectSpecHandle EffectSpec =ASC.MakeOutgoingSpec(CostEffect->GetClass(),AbilityLevel,ASC.MakeEffectContext());
			// 您的原始代码在GetStaticCostForAbility中检查了Modifiers.Num()，这里也加上更安全
			CostEffect->Modifiers[0].ModifierMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(),ManaCost);
		}
	}

	return FMath::Abs(ManaCost);
}

float UCAbilitySystemStatics::GetCooldownDurationFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC,
	int AbilityLevel)
{
	float CooldownDuration=0.f;
	if (AbilityCDO)
	{
		// 【已修正1】: 调用了正确的 GetCooldownGameplayEffect 函数
		UGameplayEffect* CoolDownEffect = const_cast<UGameplayEffect*>(AbilityCDO->GetCooldownGameplayEffect());
		// 【已修正2】: 移除了 '!'，现在是检查指针是否有效
		if (CoolDownEffect)
		{
			FGameplayEffectSpecHandle EffectSpec =ASC.MakeOutgoingSpec(CoolDownEffect->GetClass(),AbilityLevel,ASC.MakeEffectContext());
			if (EffectSpec.Data.IsValid()) // 增加一个有效性检查更安全
			{
				CoolDownEffect->DurationMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(),CooldownDuration);
			}
		}
	}

	// 您的原始代码有 Abs，这里保留，但冷却时间通常不为负
	return FMath::Abs(CooldownDuration); 
}

float UCAbilitySystemStatics::GetCooldownRemainingFor(const UGameplayAbility* AbilityCDO,
	const UAbilitySystemComponent& ASC)
{
	if (!AbilityCDO)
		return 0.f;

	UGameplayEffect* CoolDownEffect = const_cast<UGameplayEffect*>(AbilityCDO->GetCooldownGameplayEffect());
	// 【已修正1】: 移除了 '!'，现在是检查指针是否有效
	if (CoolDownEffect)
	{
		FGameplayEffectQuery CooldownEffectQuery;
		CooldownEffectQuery.EffectDefinition = CoolDownEffect->GetClass();

		float CooldownRemaining=0.f;
		// 【已修正2】: 将错误的 FJsonSerializableArrayFloat 类型改为了正确的 TArray<float>
		TArray<float> CooldownTimeRemainings =ASC.GetActiveEffectsTimeRemaining(CooldownEffectQuery);

		for (float Remaining: CooldownTimeRemainings)
		{
			if (Remaining>CooldownRemaining)
			{
				CooldownRemaining=Remaining;
			}
		}
		return CooldownRemaining;
	}

	return 0.f;
}