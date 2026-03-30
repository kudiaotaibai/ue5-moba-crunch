// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/ItemWidget.h"
#include "InventoryItemWidget.generated.h"

class UInventoryItemWidget;
class UInventoryItemDragDropOp;
struct FInventoryItemHandle;
class UInventoryItem;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemDropped,UInventoryItemWidget* ,UInventoryItemWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnButtonClick,const FInventoryItemHandle&);
/**
 * 
 */
UCLASS()
class CRUNCH_API UInventoryItemWidget : public UItemWidget
{
	GENERATED_BODY()
public:
	
	FOnInventoryItemDropped OnInventoryItemDropped;
	FOnButtonClick OnLeftButtonClicked;
	FOnButtonClick OnRightButtonClicked;
	virtual void NativeConstruct() override;
	bool IsEmpty() const;
	void SetSlotNumber(int NewSlotNumber);
	void UpdateInventoryItem(const UInventoryItem* Item);
	void EmptySlot();
	FORCEINLINE int GetSlotNumber() const {return SlotNumber;}
	void UpdateStackCount();
	UTexture2D* GetIconTexture() const;

	FORCEINLINE const UInventoryItem* GetInventoryItem()const {return InventoryItem;}
	FInventoryItemHandle GetItemHandle()const;
private:

	UPROPERTY(EditDefaultsOnly,Category="Visual")
	UTexture2D* EmptyTexture;
	
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* StackCountText;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* CooldownCountText;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* CooldownDurationText;
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* ManaCostText;

	UPROPERTY()
	const UInventoryItem* InventoryItem;

	int SlotNumber;
	virtual void RightButtonClicked() override;
	virtual void LeftButtonClicked() override;
	/*****************************/
	/*         Drag Drop         */
	/*****************************/
private:
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly,Category="Drag Drop")
	TSubclassOf<UInventoryItemDragDropOp> DragDropOpClass;
};

