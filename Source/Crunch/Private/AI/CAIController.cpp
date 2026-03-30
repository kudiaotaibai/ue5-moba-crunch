// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CAIController.h"


#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Perception/AIPerceptionComponent.h"

#include "Perception/AISenseConfig_Sight.h"

ACAIController::ACAIController()
{
	AIPerceptionComponent=CreateDefaultSubobject<UAIPerceptionComponent>("AI Perception Component");
	SightConfig=CreateDefaultSubobject<UAISenseConfig_Sight>("Sight config");

	SightConfig->DetectionByAffiliation.bDetectEnemies=true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies=false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals=false;

	SightConfig->SightRadius=1000.f;
	SightConfig->LoseSightRadius=1200.f;

	SightConfig->SetMaxAge(5.f);

	SightConfig->PeripheralVisionAngleDegrees=180.f;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this,&ACAIController::TargetPerceptionUpdated);

	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this,&ACAIController::TargetForgotten);
}

void ACAIController::BeginPlay()
{
	Super::BeginPlay();
	RunBehaviorTree(BehaviorTree);

	
}

void ACAIController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);
	IGenericTeamAgentInterface* PawnTeamInterface=Cast<IGenericTeamAgentInterface>(NewPawn);
	if (PawnTeamInterface)
	{
		SetGenericTeamId(PawnTeamInterface->GetGenericTeamId());
		ClearAndDisableAllSenses();
		EnableAllSenses();

		
	}

	UAbilitySystemComponent* PawnASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn);
	if (PawnASC)
	{
		PawnASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatTag()).AddUObject(this,&ACAIController::PawnDeadTagUpdated);
		PawnASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatTag()).AddUObject(this,&ACAIController::PawnStunTagUpdated);

	}
}

void ACAIController::TargetPerceptionUpdated(AActor* TargetActor, FAIStimulus Stimulus)
{
	AActor* CurrentTarget = const_cast<AActor*>(Cast<AActor>(GetCurrentTarget()));
	if (CurrentTarget)
	{
		const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CurrentTarget);
		// 如果当前目标存在且有"stats.dead"标签
		if (TargetASC && TargetASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatTag()))
		{
			SetCurrentTarget(nullptr); // 清空当前目标
		}
	}

	// 检查新感知的Actor是否是活的
	const UAbilitySystemComponent* SensedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (SensedASC && SensedASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatTag()))
	{
		// 如果新感知的Actor已经死亡，直接忽略，不把它设为目标
		return;
	}

	// 沿用原有逻辑，但在检查后，GetCurrentTarget() 可能已经为 nullptr
	if (Stimulus.WasSuccessfullySensed())
	{
		if (!GetCurrentTarget())
		{
			SetCurrentTarget(TargetActor);
		}
	}
}

void ACAIController::TargetForgotten(AActor* ForgottenActor)
{
	if (!ForgottenActor)
	{
		return;
	}
	if (GetCurrentTarget()==ForgottenActor)
	{
		SetCurrentTarget(GetNextPerceivedActor());
	}
}

const UObject* ACAIController::GetCurrentTarget() const
{
	const UBlackboardComponent* BlackboardComponent=GetBlackboardComponent();
	if (BlackboardComponent)
	{
		return GetBlackboardComponent()->GetValueAsObject(TargetBlackboardKeyName);
	}
	return nullptr;
}

void ACAIController::SetCurrentTarget(AActor* NewTarget)
{
     UBlackboardComponent* BlackboardComponent=GetBlackboardComponent();
	if (!BlackboardComponent)return;

	if (NewTarget)
	{
		BlackboardComponent->SetValueAsObject(TargetBlackboardKeyName, NewTarget);
	}
	else
	{
		BlackboardComponent->ClearValue(TargetBlackboardKeyName);
	}
}

AActor* ACAIController::GetNextPerceivedActor() const
{
	if (PerceptionComponent)
	{
		TArray<AActor*> Actors;
		AIPerceptionComponent->GetPerceivedHostileActors(Actors);

		if (Actors.Num() !=0)
		{
			return Actors[0];
			
		}
	}
	return nullptr;
}

void ACAIController::ForgetActorIfDead(AActor* ActorToForget)
{
	const UAbilitySystemComponent* ActorASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActorToForget);
	if (!ActorASC)
	{
		return;
	}
	if (ActorASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDeadStatTag()))
	{
		for (UAIPerceptionComponent::TActorPerceptionContainer::TIterator Iter =AIPerceptionComponent->GetPerceptualDataIterator();Iter;++Iter)
		{
			if (Iter.Key()!=ActorToForget)
			{
				continue;
			}
			for (FAIStimulus& Stimuli : Iter->Value.LastSensedStimuli)
			{
				Stimuli.SetStimulusAge(TNumericLimits<float>::Max());
			}
		}
	}
}

void ACAIController::ClearAndDisableAllSenses()
{
	AIPerceptionComponent->AgeStimuli(TNumericLimits<float>::Max());
	for (auto SenseConfight=AIPerceptionComponent->GetSensesConfigIterator();SenseConfight;++SenseConfight)
	{
		AIPerceptionComponent->SetSenseEnabled((*SenseConfight)->GetSenseImplementation(),false);
	}
	if (GetBlackboardComponent())
	{
		GetBlackboardComponent()->ClearValue(TargetBlackboardKeyName);
	}
		
}

void ACAIController::EnableAllSenses()
{
	for (auto SenseConfight=AIPerceptionComponent->GetSensesConfigIterator();SenseConfight;++SenseConfight)
	{
		AIPerceptionComponent->SetSenseEnabled((*SenseConfight)->GetSenseImplementation(),true);
	}
}

void ACAIController::PawnDeadTagUpdated(const FGameplayTag Tag, int32 Count)
{
	if (Count!=0)
	{
		GetBrainComponent()->StopLogic("Dead");
		ClearAndDisableAllSenses();
		bIsPawnDead=true;
	}
	else
	{
		GetBrainComponent()->StartLogic();
		EnableAllSenses();
		bIsPawnDead=false;
	}
}

void ACAIController::PawnStunTagUpdated(const FGameplayTag Tag, int32 Count)
{
	if (bIsPawnDead)
		return;

	if (Count!=0)
	{
		GetBrainComponent()->StopLogic("Stun");
	}
	else
	{
		GetBrainComponent()->StartLogic();
	}
}
