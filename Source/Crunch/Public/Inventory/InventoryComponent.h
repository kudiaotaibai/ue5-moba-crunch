// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItem.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"


class UPA_ShopItem;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelagate,const UInventoryItem*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemRemovedDelagate,const FInventoryItemHandle&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemStackCountChangedDelagate,const FInventoryItemHandle&,int);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRUNCH_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();
	FOnItemAddedDelagate OnItemAdded;
	FOnItemRemovedDelagate OnItemRemoved;
	FOnItemStackCountChangedDelagate OnItemStackCountChanged;
	
	void TryActivateItem(const FInventoryItemHandle& ItemHandle);
	void TryPurchase(const UPA_ShopItem* ItemToPurchase);
	void SellItem(const FInventoryItemHandle& ItemHandle);
	float GetGold() const;

	FORCEINLINE int GetCapacity() const { return Capacity; }
	void ItemSlotChanged(const FInventoryItemHandle&  Handle,int NewSlotNumber);
	UInventoryItem* GetInventoryItemByHandle(const FInventoryItemHandle& Handle) const;

	bool IsFullFor(const UPA_ShopItem* Item)const;
	bool IsAllSlotOccupied() const;
	
	UInventoryItem* GetAvaliableleStackForItem(const UPA_ShopItem* Item)const;
	bool FindIngredientForItem(const UPA_ShopItem* Item,TArray<UInventoryItem*>& OutIngredients,const TArray<const UPA_ShopItem*>& IngredientToIgnore= TArray<const UPA_ShopItem*>{}) ;
	UInventoryItem* TryGetItemForShopItem(const UPA_ShopItem* Item);
	// Called when the game starts
	virtual void BeginPlay() override;

	void TryActivateItemInSlot(int SlotNumber);
	
	
private:
	// Called every frame
	UPROPERTY(EditDefaultsOnly,Category="Inventory")
	int Capacity =6;
	
	
	
	UPROPERTY()
	UAbilitySystemComponent* OwnerAbilitySystemComponent;

	UPROPERTY()
	TMap<FInventoryItemHandle,UInventoryItem*> InventoryMap;
	
	/*********************************************/
	/*                 Server                    */
	/*********************************************/
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Purchase(const UPA_ShopItem* ItemToPurchase);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ActivateItem( const FInventoryItemHandle& ItemHandle);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SellItem( const FInventoryItemHandle& ItemHandle);
	void GrantItem(const UPA_ShopItem* NewItem);
	void ConsumeItem(UInventoryItem* Item);
	void RemoveItem(UInventoryItem* Item);
	bool TryItemCombination(const UPA_ShopItem* NewItem);
	/*********************************************/
	/*                 Client                    */
	/*********************************************/

private:
	UFUNCTION(Client,Reliable)
	void Client_ItemAdded(FInventoryItemHandle AssignedHandle,const UPA_ShopItem* Item);

	UFUNCTION(Client,Reliable)
	void Client_ItemRemoved(FInventoryItemHandle ItemHandle);
	
	UFUNCTION(Client,Reliable)
	void Client_ItemStackCountChanged(FInventoryItemHandle Handle,int NewCount);
};
