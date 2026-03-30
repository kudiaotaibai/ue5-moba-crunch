// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/LoddyGameMode.h"
#include "Network/CGameSession.h"
#include "Player/LobbyPlayerController.h"


ALoddyGameMode::ALoddyGameMode()
{
	bUseSeamlessTravel = true;
	GameSessionClass =ACGameSession::StaticClass();
	
}
  