// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GAP_Dead.generated.h"

/**
 * 
 */
UCLASS()
class CRUNCH_API UGAP_Dead : public UCGameplayAbility
{
	GENERATED_BODY()
public:

	UGAP_Dead();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:

	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float RewardRange=1000.f;
	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float BaseExperienceReward=200.f;
	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float BaseGoldReward=200.f;
	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float ExperienceRewardPerExperience=0.1f;

	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float GoldRewardPerExperience=0.05f;

	UPROPERTY(EditDefaultsOnly,Category="Raward")
	float KillerRewardPortion=0.5f;

	

	TArray<AActor*> GetRewardTargets() const;

	UPROPERTY(EditDefaultsOnly,Category="Raward")
	TSubclassOf<UGameplayEffect> RewardEffect;

	;
	
	
	
};
