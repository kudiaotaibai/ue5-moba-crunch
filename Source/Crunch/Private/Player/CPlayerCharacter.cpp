// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CPlayerCharacter.h"
#include "GAS/CAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Crunch/Crunch.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CHeroAttributeSet.h"
#include "Inventory/InventoryComponent.h"


ACPlayerCharacter:: ACPlayerCharacter()
{
	CameraBoom=CreateDefaultSubobject<USpringArmComponent>("Camera Boom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation=true;
	CameraBoom->ProbeChannel=ECC_SpringArm;

	
	ViewCam=CreateDefaultSubobject<UCameraComponent>("View Cam");
	ViewCam->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);

	bUseControllerRotationYaw=false;
	GetCharacterMovement()->bOrientRotationToMovement=true;
	GetCharacterMovement()->RotationRate=FRotator(0.f,720.0f,0.0f);


	HeroAttributeSet=CreateDefaultSubobject<UCHeroAttributeSet>("Hero Attribute Set");
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("Inventory Component");
}


void ACPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	APlayerController* OwningPlayerController=GetController<APlayerController>();
	if (OwningPlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem=OwningPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
			InputSubsystem->AddMappingContext(GameplayInputMappingContext,0);
		}
	}
}


void ACPlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInputComp=Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComp)
	{
		EnhancedInputComp->BindAction(JumpInputAction,ETriggerEvent::Triggered,this,&ACPlayerCharacter::Jump);
		EnhancedInputComp->BindAction(LookInputAction,ETriggerEvent::Triggered,this,&ACPlayerCharacter::HandleLookInput);
		EnhancedInputComp->BindAction(MoveInputAction,ETriggerEvent::Triggered,this,&ACPlayerCharacter::HandleMoveInput);
		EnhancedInputComp->BindAction(LearnAbilityLeaderAction,ETriggerEvent::Started,this,&ACPlayerCharacter::LearnAbilityLeaderDown);
		EnhancedInputComp->BindAction(LearnAbilityLeaderAction,ETriggerEvent::Completed,this,&ACPlayerCharacter::LearnAbilityLeaderUp);
		
		for (const TPair<ECAbilityInputID,UInputAction*>& InputActionPair : GameplayAbilityInputActions)
		{
			EnhancedInputComp->BindAction(InputActionPair.Value,ETriggerEvent::Triggered,this,&ACPlayerCharacter::HandleAbilityInput,InputActionPair.Key);
		}

		EnhancedInputComp->BindAction(UseInventoryItemAction,ETriggerEvent::Triggered,this,&ACPlayerCharacter::UseInventoryItem);
	}
}

void ACPlayerCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation =ViewCam->GetComponentLocation();
	OutRotation=ViewCam->GetComponentRotation();
	
}

void ACPlayerCharacter::HandleLookInput(const FInputActionValue& InputActionValue)
{
	FVector2D InputVal=InputActionValue.Get<FVector2D>();
	
	AddControllerPitchInput(-InputVal.Y);
	AddControllerYawInput(InputVal.X);
}

void ACPlayerCharacter::HandleMoveInput(const FInputActionValue& InputActionValue)
{
	FVector2D InputVal=InputActionValue.Get<FVector2D>();
	InputVal.Normalize();

	AddMovementInput(GetMoveFwDir()*InputVal.Y+GetLookRightDir()*InputVal.X);
}

void ACPlayerCharacter::LearnAbilityLeaderDown(const FInputActionValue& InputActionValue)
{
	bIsLearnAbilityLearderDown=true;
	
}

void ACPlayerCharacter::LearnAbilityLeaderUp(const FInputActionValue& InputActionValue)
{
	bIsLearnAbilityLearderDown=false;
	
}

void ACPlayerCharacter::UseInventoryItem(const FInputActionValue& InputActionValue)
{
	int Value = FMath::RoundToInt(InputActionValue.Get<float>());
	InventoryComponent->TryActivateItemInSlot(Value-1);
}

void ACPlayerCharacter::HandleAbilityInput(const FInputActionValue& InputActionValue, ECAbilityInputID InputID)
{
	bool bPressed=InputActionValue.Get<bool>();
	if (bPressed&&bIsLearnAbilityLearderDown)
	{
		UpgradeAbilityWithInputID(InputID);
		return;
	}

	if (bPressed)
	{
		GetAbilitySystemComponent()->AbilityLocalInputPressed((int32)InputID);
		
	}
	else
	{
		GetAbilitySystemComponent()->AbilityLocalInputReleased((int32)InputID);
	}

	if (InputID==ECAbilityInputID::BasicAttack)
	{
		
		FGameplayTag BasicAttackTag =bPressed?UCAbilitySystemStatics::GetBasicAttackInputPressedTag():UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();
		
		
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this,BasicAttackTag,FGameplayEventData());
		Server_SendGameplayEventToSelf(BasicAttackTag,FGameplayEventData());
	}
}

void ACPlayerCharacter::SetInputEnabledFromPlayerController(bool bEnabled)
{
	APlayerController* PlayerController=GetController<APlayerController>();
	if (!PlayerController)
	{
		return;
	}
	if (bEnabled)
	{
		EnableInput(PlayerController);
	}
	else
	{
		DisableInput(PlayerController);	
	}
	
}

void ACPlayerCharacter::OnStun()
{
	SetInputEnabledFromPlayerController(false);
}

void ACPlayerCharacter::OnRecoverFromStun()
{
	if (IsDead()) return;
	SetInputEnabledFromPlayerController(true);
}

void ACPlayerCharacter::OnDead()
{
	APlayerController* PlayerController=GetController<APlayerController>();
	if (PlayerController)
	{
		DisableInput(PlayerController);
	}
}

void ACPlayerCharacter::OnRespawn()
{
	APlayerController* PlayerController=GetController<APlayerController>();
	if (PlayerController)
	{
		EnableInput(PlayerController);
	}
}



FVector ACPlayerCharacter::GetLookRightDir() const
{
	return ViewCam->GetRightVector();
}
FVector ACPlayerCharacter::GetLookFwDir() const
{
	return ViewCam->GetForwardVector();
}
FVector ACPlayerCharacter::GetMoveFwDir() const
{
	return FVector::CrossProduct(GetLookRightDir(),FVector::UpVector);
}

void ACPlayerCharacter::OnAimStatChanged(bool bIsAiming)
{
	
	LerpCameraToLocalOffsetLocation(bIsAiming?CameraAimLocalOffset:FVector{0.f});
}



void ACPlayerCharacter::LerpCameraToLocalOffsetLocation(const FVector& Goal)
{
	GetWorldTimerManager().ClearTimer(CameraLerpTimerHandle);
	CameraLerpTimerHandle=GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this,&ACPlayerCharacter::TickCameraLocalOffsetLerp,Goal));
}

void ACPlayerCharacter::TickCameraLocalOffsetLerp(FVector Goal)
{
	FVector CurrentLocalOffset=ViewCam->GetRelativeLocation();

	if (FVector::Dist(CurrentLocalOffset,Goal)<1.f )
	{
		ViewCam->SetRelativeLocation(Goal);
		return;
	}

	float LerpAlpha=FMath::Clamp(GetWorld()->GetDeltaSeconds()*CameraLerpSpeed,0.f,1.f);
	FVector NewLocalOffset=FMath::Lerp(CurrentLocalOffset,Goal,LerpAlpha);
	ViewCam->SetRelativeLocation(NewLocalOffset);

	CameraLerpTimerHandle=GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this,&ACPlayerCharacter::TickCameraLocalOffsetLerp,Goal));

}