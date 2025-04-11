// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Experimental/RockInventory_Slot_ItemBase.h"

#include "Components/TextBlock.h"
#include "Inventory/RockInventory.h"
#include "Library/RockInventoryLibrary.h"
#include "Item/RockItemStack.h"

void URockInventory_Slot_ItemBase::NativeConstruct()
{
    Super::NativeConstruct();
    
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
    
    Super::NativeDestruct();
}

void URockInventory_Slot_ItemBase::UpdateItemCount()
{
    if (!Inventory || !ItemCount || !SlotHandle.IsValid())
    {
        return;
    }
    
    // Get the item at this slot
    FRockItemStack ItemStack;
    if (URockInventoryLibrary::GetItemAtLocation(Inventory, SlotHandle, ItemStack))
    {
        // Update the count text if stack size is greater than 1
        if (ItemStack.StackSize > 1)
        {
            ItemCount->SetText(FText::AsNumber(ItemStack.StackSize));
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
