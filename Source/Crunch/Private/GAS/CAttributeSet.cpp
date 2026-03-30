// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


void UCAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,Health,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,MaxHealth,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,Mana,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,MaxMana,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,AttackDamage,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,Armor,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet,MoveSpeed,COND_None,REPNOTIFY_Always);
	
}

void UCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute==GetHealthAttribute())
	{
		NewValue=FMath::Clamp(NewValue,0.f,GetMaxHealth());
	}
	if (Attribute==GetManaAttribute())
	{
		NewValue=FMath::Clamp(NewValue,0.f,GetMaxMana());
	}
}

void UCAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute==GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(),0.f,GetMaxHealth()));
		SetCachedHealthPercent(GetHealth()/GetMaxHealth());
	}
	if (Data.EvaluatedData.Attribute==GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(),0.f,GetMaxMana()));
		SetCachedManaPercent(GetMana()/GetMaxMana());

	}
}

void UCAttributeSet::RescaleHealth()
{
	const AActor* OwningActor = GetOwningActor();
    	if (!OwningActor || !OwningActor->HasAuthority())
    	{
    		return;
    	}
    
    	if(GetCachedHealthPercent() != 0 && GetHealth() != 0)
    	{
    		SetHealth(GetMaxHealth() * GetCachedHealthPercent());
    	}
}

void UCAttributeSet::RescaleMana()
{
	const AActor* OwningActor = GetOwningActor();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	if(GetCachedManaPercent() != 0 && GetMana() != 0)
	{
		SetMana(GetMaxMana() * GetCachedManaPercent());
	}
}


void UCAttributeSet::OnRep_Health(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,Health,OldValue)
	
}

void UCAttributeSet::OnRep_MaxHealth(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,MaxHealth,OldValue)
}

void UCAttributeSet::OnRep_Mana(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,Mana,OldValue)
}

void UCAttributeSet::OnRep_MaxMana(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,MaxMana,OldValue)
}

 void UCAttributeSet::OnRep_AttackDamage(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,AttackDamage,OldValue)
}

 void UCAttributeSet::OnRep_Armor(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,Armor,OldValue)
}

 void UCAttributeSet::OnRep_MoveSpeed(FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet,MoveSpeed,OldValue)
}
