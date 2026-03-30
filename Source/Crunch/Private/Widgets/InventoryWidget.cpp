// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Widgets/InventoryItemWidget.h"
#include "Components/WrapBox.h"
#include "Widgets/InventoryContextMenuWidget.h"
#include "Components/WrapBoxSlot.h"

void UInventoryWidget::NativeConstruct()
{
	
	Super::NativeConstruct();
	if (APawn* OwnerPawn=GetOwningPlayerPawn())
	{
		InventoryComponent = OwnerPawn->GetComponentByClass<UInventoryComponent>();
		if (InventoryComponent)
		{
			InventoryComponent->OnItemAdded.AddUObject(this,&UInventoryWidget::ItemAdded);
			InventoryComponent->OnItemRemoved.AddUObject(this,&UInventoryWidget::ItemRemoved);
			InventoryComponent->OnItemStackCountChanged.AddUObject(this,&UInventoryWidget::ItemStackCountChanged);
			int Capacity =InventoryComponent->GetCapacity();
				
			ItemList->ClearChildren();
			for (int i = 0; i<Capacity; ++i)
			{
				UInventoryItemWidget* NewEmptyWidget =CreateWidget<UInventoryItemWidget>(GetOwningPlayer(),ItemWidgetClass);
				if (NewEmptyWidget)
				{
					NewEmptyWidget->SetSlotNumber(i);
					UWrapBoxSlot* NewItemSlot =  ItemList->AddChildToWrapBox(NewEmptyWidget);
					NewItemSlot->SetPadding(FMargin(2.f));
					ItemWidgets.Add(NewEmptyWidget);

					NewEmptyWidget->OnInventoryItemDropped.AddUObject(this,&UInventoryWidget::HandleItemDragDrop);
					NewEmptyWidget->OnLeftButtonClicked.AddUObject(InventoryComponent,&UInventoryComponent::TryActivateItem);
					NewEmptyWidget->OnRightButtonClicked.AddUObject(this,&UInventoryWidget::ToggleContextMenu);
				}
			}

			SpawnContextMenu();
		}
	}
}

void UInventoryWidget::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath,
	const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
		if (!NewWidgetPath.ContainsWidget(ContextMenuWidget->GetCachedWidget().Get()))
		{
			ClearContextMenu();
		}
}

void UInventoryWidget::SpawnContextMenu()
{
	if (!ContextMenuWidgetClass)
		return;

	ContextMenuWidget = CreateWidget<UInventoryContextMenuWidget>(this,ContextMenuWidgetClass);
	if (ContextMenuWidget)
	{
		ContextMenuWidget->GetSellButtonClickedEvent().AddDynamic(this,&UInventoryWidget::SellFocusedItem);
		ContextMenuWidget->GetUseButtonClickedEvent().AddDynamic(this,&UInventoryWidget::UseFocusedItem);
		ContextMenuWidget->AddToViewport(1);
		SetContextMenuVisible(false);
	}
}

void UInventoryWidget::SellFocusedItem()
{
	InventoryComponent->SellItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}

void UInventoryWidget::UseFocusedItem()
{
	InventoryComponent->TryActivateItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}

void UInventoryWidget::SetContextMenuVisible(bool bContextMenuVisible)
{
	if (ContextMenuWidget)
	{
		ContextMenuWidget->SetVisibility(bContextMenuVisible? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		return;
	}
	
}

void UInventoryWidget::ToggleContextMenu(const FInventoryItemHandle& ItemHandle)
{
	if (CurrentFocusedItemHandle == ItemHandle)
	{
		ClearContextMenu();
		return;
	}

	CurrentFocusedItemHandle = ItemHandle;
	UInventoryItemWidget** ItemWidgetPtrPtr = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (!ItemWidgetPtrPtr)
		return;

	UInventoryItemWidget* ItemWidget = *ItemWidgetPtrPtr;
	if (!ItemWidget)
		return;

	SetContextMenuVisible(true);
	FVector2D ItemAbsPos =ItemWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D{1.f,0.5f});
	FVector2D ItemWidgetPixelPos,ItemWidgetViewportPos;
	USlateBlueprintLibrary::AbsoluteToViewport(this,ItemAbsPos,ItemWidgetPixelPos,ItemWidgetViewportPos);

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (OwningPlayerController)
	{
		int ViewportSizeX, ViewportSizeY;
		OwningPlayerController->GetViewportSize(ViewportSizeX,ViewportSizeY);
		float Scale =  UWidgetLayoutLibrary::GetViewportScale(this);

		int Overshoot = ItemWidgetPixelPos.Y + ContextMenuWidget->GetDesiredSize().Y* Scale-ViewportSizeY;
		if (Overshoot >0)
		{
			ItemWidgetPixelPos.Y -= Overshoot;
		}
	}
	ContextMenuWidget->SetPositionInViewport(ItemWidgetViewportPos);
}

void UInventoryWidget::ClearContextMenu()
{
	ContextMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	CurrentFocusedItemHandle = FInventoryItemHandle::InvalidHandle();
}


void UInventoryWidget::ItemAdded(const UInventoryItem* InventoryItem)
{
	if (!InventoryItem)
		return;

	if (UInventoryItemWidget* NextAvaliableSlot = GetNextAvaliableSlot())
	{
		NextAvaliableSlot->UpdateInventoryItem(InventoryItem);
		PopulatedItemEntryWidgets.Add(InventoryItem->GetHandle(),NextAvaliableSlot);
		if (InventoryComponent)
		{
			InventoryComponent->ItemSlotChanged(InventoryItem->GetHandle(),NextAvaliableSlot->GetSlotNumber());
		}
	}
}

void UInventoryWidget::ItemStackCountChanged(const FInventoryItemHandle& Handle, int NewCount)
{
	UInventoryItemWidget** FoundWidget = PopulatedItemEntryWidgets.Find(Handle);
	if (FoundWidget)
	{
		(*FoundWidget)->UpdateStackCount();
	}
}

UInventoryItemWidget* UInventoryWidget::GetNextAvaliableSlot() const
{
	for (UInventoryItemWidget* Widget :ItemWidgets)
	{
		if (Widget->IsEmpty())
		{
			return Widget;
		}
	}

	return nullptr;
}

// --- 文件：InventoryWidget.cpp (修正后的版本) ---
void UInventoryWidget::HandleItemDragDrop(UInventoryItemWidget* DestinationWidget, UInventoryItemWidget* SourceWidget)
{
	if (!SourceWidget || !DestinationWidget || !InventoryComponent)
	{
		return; // 安全检查，防止意外情况
	}

	// 1. 获取拖拽前的数据状态
	const UInventoryItem* SrcItem = SourceWidget->GetInventoryItem();
	const UInventoryItem* DstItem = DestinationWidget->GetInventoryItem();

	// 源物品必须存在才能发起拖拽
	if (!SrcItem)
	{
		return;
	}

	// 2. 更新后端数据 (InventoryComponent)
	// 源物品的新格子编号现在是目标格子的编号
	InventoryComponent->ItemSlotChanged(SrcItem->GetHandle(), DestinationWidget->GetSlotNumber());
	
	// 检查目标格子是否原本有物品（即交换情况）
	if (DstItem)
	{
		// 如果是交换，则目标物品的新格子编号是源格子的编号
		InventoryComponent->ItemSlotChanged(DstItem->GetHandle(), SourceWidget->GetSlotNumber());
	}

	// 3. 更新UI查找表 (PopulatedItemEntryWidgets)
	// 将源物品的Handle映射到新的目标Widget
	PopulatedItemEntryWidgets.Add(SrcItem->GetHandle(), DestinationWidget);
	
	if (DstItem)
	{
		// 如果是交换，将目标物品的Handle映射到新的源Widget
		PopulatedItemEntryWidgets.Add(DstItem->GetHandle(), SourceWidget);
	}
	else
	{
		// 如果目标格子是空的，那么源物品的旧映射会被上面的 Add 操作覆盖，
		// 无需额外处理。
	}

	// 4. 最后更新两个Widget的视觉显示
	// UpdateInventoryItem 函数可以正确处理传入 nullptr 的情况（会调用 EmptySlot）
	DestinationWidget->UpdateInventoryItem(SrcItem);
	SourceWidget->UpdateInventoryItem(DstItem);
}
void UInventoryWidget::ItemRemoved(const FInventoryItemHandle& ItemHandle)
{
	UInventoryItemWidget** FoundWidget = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (FoundWidget&&*FoundWidget)
	{
		(*FoundWidget)->EmptySlot();
		PopulatedItemEntryWidgets.Remove(ItemHandle);
	}
}
