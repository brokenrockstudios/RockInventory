// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockInventory_Slot_ItemBase.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockInventoryLibrary.h"
#include "Item/RockItemStack.h"
#include "Library/RockInventoryManagerLibrary.h"
#include "Transactions/Implementations/RockMoveItemTransaction.h"
#include "UI/RockItemDragDropOperation.h"
#include "UObject/ICookInfo.h"

void URockInventory_Slot_ItemBase::NativeConstruct()
{
	Super::NativeConstruct();

	bIsCurrentlyLoading = false;
	// Hide count by default if we haven't yet set it
	if (ItemCount)
	{
		ItemCount->SetVisibility(ESlateVisibility::Hidden);
	}

	// Register for inventory change events
	if (Inventory)
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &URockInventory_Slot_ItemBase::OnInventoryChanged);
		// Update the item count on construction
		UpdateItemCount();
	}
}

void URockInventory_Slot_ItemBase::NativeDestruct()
{
	// Unregister from inventory change events
	if (Inventory)
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &URockInventory_Slot_ItemBase::OnInventoryChanged);
	}
	// Clean up any pending loads when widget is destroyed
	if (StreamHandle.IsValid() && !StreamHandle->HasLoadCompleted())
	{
		StreamHandle->CancelHandle();
		StreamHandle.Reset();
	}

	Super::NativeDestruct();
}

void URockInventory_Slot_ItemBase::UpdateItemCount()
{
	if (!Inventory || !ItemCount || !SlotHandle.IsValid())
	{
		return;
	}

	// Get the item at this slot
	const FRockItemStack& ItemStack = URockInventoryLibrary::GetItemAtLocation(Inventory, SlotHandle);
	if (ItemStack.IsValid() && ItemStack.IsOccupied())
	{
		// Update the count text if stack size is greater than 1
		if (ItemStack.GetStackSize() > 1)
		{
			ItemCount->SetText(FText::AsNumber(ItemStack.GetStackSize()));
			ItemCount->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// Hide count for single items
			ItemCount->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		// No item in this slot
		ItemCount->SetVisibility(ESlateVisibility::Hidden);
	}
}

void URockInventory_Slot_ItemBase::OnInventoryChanged(URockInventory* ChangedInventory, const FRockInventorySlotHandle& ChangedSlotHandle)
{
	// Only update if this change affects our slot
	if (ChangedInventory == Inventory &&
		(ChangedSlotHandle == SlotHandle || !ChangedSlotHandle.IsValid()))
	{
		UpdateItemCount();
	}
}

bool URockInventory_Slot_ItemBase::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const URockItemDragDropOperation* DragDropOp = Cast<URockItemDragDropOperation>(InOperation);
	if (!DragDropOp)
	{
		return false;
	}
	
	URockMoveItemTransaction* MoveTransaction = URockMoveItemTransaction::CreateMoveItemTransaction(
		DragDropOp->SourceInventory, DragDropOp->SourceSlot,
		Inventory, SlotHandle);

	return URockInventoryManagerLibrary::EnqueueTransaction(GetOwningPlayer(), MoveTransaction);
}

void URockInventory_Slot_ItemBase::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	// Update the inventory as the mouse is currently over this, and to help with updating drag/drop indicators
	// This Item, will obstruct the mouse event from reaching the BG base, so we basically have duplicate code in this AND BG base.
	

	
	// We could in theory either have a common parent, have some dupe code, or make all items 'non hit testable' and just have the BG base handle it.
	// Verify this, perhaps if the base has NativeOnDragEnter, and this doesn't, maybe we can just call the base?
}

void URockInventory_Slot_ItemBase::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	// Update the inventory as the mouse is currently over this, and to help with updating drag/drop indicators
}

void URockInventory_Slot_ItemBase::SetIconData(const FRockItemUIData& InIconData)
{
	IconData = InIconData;
	// TODO: Use the IconData to set hover/tooltip?

	SetIsLoading(true);

	// Handle null case
	if (IconData.Icon.IsNull())
	{
		// Set fallback texture
		if (FallbackIcon)
		{
			ItemIcon->SetBrushFromTexture(FallbackIcon);
		}
		else
		{
			ItemIcon->SetBrushResourceObject(nullptr);
		}
		SetIsLoading(false);
		return;
	}

	// If already loaded, use it immediately
	if (IconData.Icon.IsValid())
	{
		ItemIcon->SetBrushFromTexture(IconData.Icon.Get());
		SetIsLoading(false);
		return;
	}

	// Cancel any existing stream handle
	if (StreamHandle.IsValid() && !StreamHandle->HasLoadCompleted())
	{
		StreamHandle->CancelHandle();
	}

	// Start async load
	StreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		IconData.Icon.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &URockInventory_Slot_ItemBase::OnIconLoaded)
	);
}

void URockInventory_Slot_ItemBase::SetIsLoading(bool bIsLoading)
{
	bIsCurrentlyLoading = bIsLoading;

	// Update loading indicator visibility
	if (LoadingIndicator)
	{
		LoadingIndicator->SetVisibility(bIsLoading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Optionally apply any visual changes to the item icon during loading
	if (ItemIcon)
	{
		// Could adjust opacity, visibility, etc.
		// For example:
		ItemIcon->SetRenderOpacity(bIsLoading ? 0.3f : 1.0f);
	}
}

void URockInventory_Slot_ItemBase::OnIconLoaded()
{
	// Only proceed if we're still valid
	if (!IsValidLowLevel())
	{
		return;
	}

	if (IconData.Icon.IsValid())
	{
		ItemIcon->SetBrushFromTexture(IconData.Icon.Get());
	}
	else
	{
		// Set fallback texture
		if (FallbackIcon)
		{
			ItemIcon->SetBrushFromTexture(FallbackIcon);
		}
		else
		{
			ItemIcon->SetBrushResourceObject(nullptr);
		}
	}

	SetIsLoading(false);
	StreamHandle.Reset();
}
