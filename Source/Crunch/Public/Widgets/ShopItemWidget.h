

#pragma once

#include "CoreMinimal.h"
#include "Widgets/ItemWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ShopItemWidget.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemPurchaseIssued, const UPA_ShopItem*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopItemSelected, const UShopItemWidget*);
/**
 * 
 */
UCLASS()
class CRUNCH_API UShopItemWidget : public UItemWidget,public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	FOnItemPurchaseIssued OnItemPurchaseIssued;
	FOnShopItemSelected OnShopItemClicked;
	
	
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	FORCEINLINE const UPA_ShopItem* GetShopItem() const{return  ShopItem;} 

private:
	UPROPERTY()
	const UPA_ShopItem* ShopItem;
	
	virtual void RightButtonClicked() override;
	virtual void LeftButtonClicked() override;
	
};
