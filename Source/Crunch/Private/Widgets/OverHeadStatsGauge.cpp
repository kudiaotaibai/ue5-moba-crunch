// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/OverHeadStatsGauge.h"
#include "AbilitySystemComponent.h"

#include "GAS/CAttributeSet.h"
#include "Widgets/ValueGauge.h"


void UOverHeadStatsGauge::ConfigureWithASC(class UAbilitySystemComponent* AbilitySystemComponent)
{
if (AbilitySystemComponent)
	{
		HealthBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent,UCAttributeSet::GetHealthAttribute(),UCAttributeSet::GetMaxHealthAttribute());
		ManaBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent,UCAttributeSet::GetManaAttribute(),UCAttributeSet::GetMaxManaAttribute());

	}
	
}
