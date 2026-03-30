// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SkeletalMeshRenderWidget.h"
#include "GameFramework/Character.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Widgets/RenderActorTargetInterface.h"
#include "Widgets/SkeletalMeshRenderActor.h"


void USkeletalMeshRenderWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ACharacter* PlayerCharacter = GetOwningPlayerPawn<ACharacter>();
	IRenderActorTargetInterface* PlayerCharacterRenderTargetInterface =Cast<IRenderActorTargetInterface>(PlayerCharacter);
	
	if (PlayerCharacter&&SkeletalMeshRenderActor)
	{
		SkeletalMeshRenderActor->ConfigureSkeletalMesh(PlayerCharacter->GetMesh()->GetSkeletalMeshAsset(),PlayerCharacter->GetMesh()->GetAnimClass());
		USceneCaptureComponent2D* SceneCapture = SkeletalMeshRenderActor->GetCaptureComponent();
		if (PlayerCharacterRenderTargetInterface&&SceneCapture)
		{
			SceneCapture->SetRelativeLocation(PlayerCharacterRenderTargetInterface->GetCaptureLocalPositon());
			SceneCapture->SetRelativeRotation(PlayerCharacterRenderTargetInterface->GetCaptureLocalRotation());
		}
	}
}

void USkeletalMeshRenderWidget::SpawnRenderActor()
{
	if (!SkeletalMeshRenderActorClass)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SkeletalMeshRenderActor=World->SpawnActor<ASkeletalMeshRenderActor>(SkeletalMeshRenderActorClass, SpawnParams);
}

ARenderActor* USkeletalMeshRenderWidget::GetRenderActor() const
{
	return SkeletalMeshRenderActor;
}
