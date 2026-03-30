// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "OnlineSessionSettings.h"
#include "MainMenuWidget.generated.h"

/**
 * 
 */

class FOnButtonClickedEvent;

UCLASS()
class CRUNCH_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;

	/*******************************/
	/*              Main           */
	/*******************************/
private:
	UPROPERTY(meta=(BindWidget))
	class UWidgetSwitcher* MainSwitcher;

	UPROPERTY()
	class UCGameInstance* CGameInstance;
	
	void SwitchToMainWidget();

	UPROPERTY(meta=(BindWidget))
	class UWidget* MainWidgetRoot;

	void  JoinSessionFailed();

	void UpdateLobbyList(const TArray<FOnlineSessionSearchResult>& SearchResults);

	UPROPERTY(meta=(BindWidget))
	class UScrollBox* SessionScrollBox;

	UPROPERTY(meta=(BindWidget))
	class UButton* JoinSessionBtn;

	UPROPERTY(EditDefaultsOnly,Category="Session")
	TSubclassOf<class USessionEntryWidget> SessionEntryWidgetClass ;

	FString CurrentSelectedSessionId ="";

	UFUNCTION()
	void  JoinSessionBtnClicked();

	void SessionEntrySelected(const FString& SelectedEntryIdStr);
	/*******************************/
	/*              Login          */
	/*******************************/
private:
	UPROPERTY(meta=(BindWidget))
	class UWidget* LoginWidgetRoot;

	UPROPERTY(meta=(BindWidget))
	class UButton* LoginBtn;

	UFUNCTION()
	void LoginBtnClicked();

	void LoginCompleted(bool bWasSuccessful,const FString& PlayerNickname,const FString& ErrorMsg);

	/*******************************/
	/*            Session          */
	/*******************************/
	UPROPERTY(meta=(BindWidget))
	class UButton* CreateSessionBtn;

	UPROPERTY(meta=(BindWidget))
	class UEditableText* NewSessionNameText;
	
	UFUNCTION()
	void CreateSessionBtnClicked();

	UFUNCTION()
	void CancelSessionCreation();
	
	UFUNCTION()
	void NewSessionNameTextChanged(const FText& NewText);


	
	/*******************************/
	/*              Waiting         */
	/*******************************/
private:
	UPROPERTY(meta=(BindWidget))
	class UWaitingWidget* WaitingWidget;

	FOnButtonClickedEvent& SwitchToWaitingWidget(const FText& WaitInfo,bool bAllowCancel = false);
	
};
