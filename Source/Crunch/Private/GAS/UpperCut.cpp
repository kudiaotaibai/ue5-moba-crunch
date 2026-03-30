// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/UpperCut.h"
#include "GameplayTagsManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/GA_Combo.h"

void UUpperCut::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo,&ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayUppCutMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,UpperCutMontage);
		PlayUppCutMontageTask->OnBlendOut.AddDynamic(this,&UUpperCut::K2_EndAbility);
		PlayUppCutMontageTask->OnCancelled.AddDynamic(this,&UUpperCut::K2_EndAbility);
		PlayUppCutMontageTask->OnInterrupted.AddDynamic(this,&UUpperCut::K2_EndAbility);
		PlayUppCutMontageTask->OnCompleted.AddDynamic(this,&UUpperCut::K2_EndAbility);
		PlayUppCutMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitLaunchEventTask=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,GetUpperCutLaunchTag());
		WaitLaunchEventTask->EventReceived.AddDynamic(this,&UUpperCut::StartLaunching);
		WaitLaunchEventTask->ReadyForActivation();
	}
	NextComboName = NAME_None;
}

UUpperCut::UUpperCut()
{
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
}

FGameplayTag UUpperCut::GetUpperCutLaunchTag()
{
		return  FGameplayTag::RequestGameplayTag("ability.uppercut.launch");
}

void UUpperCut::StartLaunching(FGameplayEventData EventData)
{
		if (K2_HasAuthority())
		{
			PushTarget(GetAvatarActorFromActorInfo(),FVector::UpVector*UpperCutLaunchSpeed);
			int HitResultCount =UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(EventData.TargetData);	
			for (int i=0;i<HitResultCount;i++)
			{
				FHitResult HitResult =UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData,i);
				PushTarget(HitResult.GetActor(),FVector::UpVector*UpperCutLaunchSpeed);
				ApplyGameplayEffectToHitResultActor(HitResult,LaunchDamageEffect,GetAbilityLevel(CurrentSpecHandle,CurrentActorInfo));

			}
		}

	UAbilityTask_WaitGameplayEvent* WaitComboChangeEvent =WaitComboChangeEvent=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,UGA_Combo::GetComboChangedEventTag(),nullptr,false,false);

	WaitComboChangeEvent->EventReceived.AddDynamic(this,&UUpperCut::HandleComboChangeEvent);
	WaitComboChangeEvent->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitComboCommitEvent=UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,UCAbilitySystemStatics::GetBasicAttackInputPressedTag());
	WaitComboCommitEvent->EventReceived.AddDynamic(this,&UUpperCut::HandleComboCommitEvent);
	WaitComboCommitEvent->ReadyForActivation();


	UAbilityTask_WaitGameplayEvent* WaitComboDamageEvent =UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,UGA_Combo::GetComboTargetEventTag());
	WaitComboDamageEvent->EventReceived.AddDynamic(this,&UUpperCut::HandleComboDamageEvent);
	WaitComboDamageEvent->ReadyForActivation();
}

void UUpperCut::HandleComboChangeEvent(FGameplayEventData EventData)
{
	FGameplayTag EventTag =EventData.EventTag;
	if (EventTag == UGA_Combo::UGA_Combo::GetComboChangedEventEndTag())
	{
		NextComboName=NAME_None;
		UE_LOG(LogTemp,Warning	,TEXT("Next Combo is cleared"));
		return;
	}

	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag,TagNames);
	NextComboName=TagNames.Last();
	UE_LOG(LogTemp,Warning	,TEXT("Next Combo Name: %s"),*NextComboName.ToString());

}

void UUpperCut::HandleComboCommitEvent(FGameplayEventData EventData)
{
	if (NextComboName==NAME_None)
	{
		return;
	}
	UAnimInstance* OwnerAnimInst=GetOwnerAnimInstance();
	if (!OwnerAnimInst)
	{
		return;
	}
	
	OwnerAnimInst->Montage_SetNextSection(OwnerAnimInst->Montage_GetCurrentSection(UpperCutMontage),NextComboName,UpperCutMontage);
}

void UUpperCut::HandleComboDamageEvent(FGameplayEventData EventData)
{
	if (K2_HasAuthority())
	{
		PushTarget(GetAvatarActorFromActorInfo(),FVector::UpVector*UpperCutLaunchSpeed);
		int HitResultCount =UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(EventData.TargetData);	
		for (int i=0;i<HitResultCount;i++)
		{
			FHitResult HitResult =UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData,i);
			PushTarget(HitResult.GetActor(),FVector::UpVector*UpperCutLaunchSpeed);
			ApplyGameplayEffectToHitResultActor(HitResult,LaunchDamageEffect,GetAbilityLevel(CurrentSpecHandle,CurrentActorInfo));

		}
	}
}

