// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerInfoTypes.h"
#include "Framework/CGameState.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"

#include "CPlayerState.generated.h"

/**
 * 
 */
class UPA_CharacterDefination;
UCLASS()
class CRUNCH_API ACPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	ACPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	TSubclassOf<APawn> GetSelectedPawnClass() const;
	FGenericTeamId GetTeamIdBasedOnSlot() const;
	
	UFUNCTION(Server, Reliable,WithValidation)
	void Server_SetSelectedCharacterDefination(const UPA_CharacterDefination* NewDefination);
	
private:
	UPROPERTY(Replicated)
	FPlayerSelection PlayerSelection;
	

	UPROPERTY()
	class ACGameState* CGameState;


	void PlayerSelectionUpdated(const TArray<FPlayerSelection>& NewPlayerSelections);
	
};

