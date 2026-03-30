// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameState.h"
#include "Net/UnrealNetwork.h"

void ACGameState::RequestPlayerSelectionChange(const APlayerState* RequestingPlayer, uint8 DesiredSlot)
{
	if (!HasAuthority()||IsSlotOccupied(DesiredSlot))
	{
		return;
	}

	FPlayerSelection* PlayerSelectPtr =PlayerSelectionArray.FindByPredicate([&](const FPlayerSelection& PlayerSelection)
	{
		return PlayerSelection.IsForPlayer(RequestingPlayer);
	}
	);

	if (PlayerSelectPtr)
	{
		PlayerSelectPtr->SetSlot(DesiredSlot);
	}
	else
	{
		PlayerSelectionArray.Add(FPlayerSelection(DesiredSlot,RequestingPlayer));
		
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

void ACGameState::SetCharacterSelected(const APlayerState* SelectingPlayer,
	const UPA_CharacterDefination* SelectedDefination)
{
	if (IsDefiniationSelected(SelectedDefination))
	{
		return;
	}

	FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[&](const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.IsForPlayer(SelectingPlayer);
		}
	);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetCharacterDefination(SelectedDefination);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

bool ACGameState::IsSlotOccupied(uint8 SlotId) const
{
	for (const FPlayerSelection& PlayerSelection : PlayerSelectionArray)
	{
		if (PlayerSelection.GetPlayerSlot() == SlotId)
		{
			return true;
		}
	}
	return false;
}

bool ACGameState::IsDefiniationSelected(const UPA_CharacterDefination* Defination) const
{
	const FPlayerSelection* FoundPlayerSelection =PlayerSelectionArray.FindByPredicate(
		[&](const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.GetCharacterDefination() == Defination;
		}
	);

	return FoundPlayerSelection != nullptr;
}

void ACGameState::SetCharacterDeselected(const UPA_CharacterDefination* DefinationToDeselect)
{

	if (!DefinationToDeselect)
		return;
	
	FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
	[&](const FPlayerSelection& PlayerSelection)
	{
		return PlayerSelection.GetCharacterDefination() ==DefinationToDeselect;
	}
);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetCharacterDefination(nullptr);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

const TArray<FPlayerSelection>& ACGameState::GetPlayerSelection() const
{
	return PlayerSelectionArray;
}

bool ACGameState::CanStartHeroSelection() const
{
	return  PlayerSelectionArray.Num() == PlayerArray.Num();
}

bool ACGameState::CanStartMatch() const
{
	for (const FPlayerSelection& PlayerSelection : PlayerSelectionArray)
	{
		if (PlayerSelection.GetCharacterDefination()==nullptr)
		{
			return false;
		}
	}

	return true;
}

void ACGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ACGameState,PlayerSelectionArray,COND_None,REPNOTIFY_Always);
}

void ACGameState::OnRep_PlayerSelectionArray()
{
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);

}
