// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Combo.generated.h"

/**
 * 
 */
UCLASS()
class CRUNCH_API UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Combo();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	static  FGameplayTag GetComboChangedEventTag();
	static  FGameplayTag GetComboChangedEventEndTag();
	static  FGameplayTag GetComboTargetEventTag();
private:
	UPROPERTY(EditDefaultsOnly,Category="Animation")
	UAnimMontage* ComboMontage;
	UPROPERTY(EditDefaultsOnly,Category="Gameplay Effect")
	TMap<FName,TSubclassOf<UGameplayEffect>> DamageEffectMap;
	UPROPERTY(EditDefaultsOnly,Category="Gameplay Effect")
	TSubclassOf<UGameplayEffect> DefaultDamageEffect;

	
	

	
	TSubclassOf<UGameplayEffect> GetDamageEffectForCurrentCombo();
	void TryCommitCombo();
	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	
	UFUNCTION()
	void SetupWaitComboInputPress();
	UFUNCTION()
	void ComboChangedEventReceived(FGameplayEventData Data);
	UFUNCTION()
	void DoDamage(FGameplayEventData Data);
	
	FName NextComboName;
};
