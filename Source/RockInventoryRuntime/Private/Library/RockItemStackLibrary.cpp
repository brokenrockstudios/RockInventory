// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockItemStackLibrary.h"

#include "RockInventoryLogging.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"

URockItemDefinition* URockItemStackLibrary::GetItemDefinition(const FName& ItemId)
{
	if (const URockItemRegistrySubsystem* ItemRegistry = URockItemRegistrySubsystem::GetInstance())
	{
		return ItemRegistry->FindDefinition(ItemId);
	}
	return nullptr;
}

FVector2D URockItemStackLibrary::GetItemSize(const FRockItemStack& ItemStack)
{
	// We shouldn't have ItemStacks without valid definitions
	checkf(ItemStack.IsValid(), TEXT("ItemStack is invalid!"));
	if (ItemStack.IsValid())
	{
		return ItemStack.Definition->SlotDimensions;
	}
	return FVector2D(1, 1);
}

FRockItemStack URockItemStackLibrary::CreateItemStack(URockInventory* OwningInventory, const FRockItemStack& InItemStack)
{
	checkf(OwningInventory, TEXT("CreateItemStack called with invalid inventory!"));
	FRockItemStack ItemStack = InItemStack;
	
	ItemStack.bIsOccupied = true;
	checkf(ItemStack.IsValid(), TEXT("CreateItemStack called with invalid item stack!"));
	if (!ItemStack.IsValid())
	{
		return FRockItemStack::Invalid();
	}
	// TODO: Build out a proper ItemRegistry
	// This would be a good time to LoadAsync certain aspects of the ItemDefinition

	// If we have an ItemId but no definition, we should look it up in the registry
	// Definition = URockItemRegistry::Get()->FindDefinition(ItemId);
	// 	checkf(ItemDefinition, TEXT("ItemStack %s has no definition set!"), *ItemStack.GetDebugString());
	// }
	//else if (ItemStack.ItemId == NAME_None && !ItemStack.Definition)
	// {
	//	UE_LOG(LogTemp, Error, TEXT("ItemStack %s has no ItemId or Definition set!"), *ItemStack.GetDebugString());
	//	return ItemStack;
	//}
	if (ItemStack.Definition->bRequiresRuntimeInstance)
	{
		// The outer should be the inventory that owns this item stack?
		ItemStack.RuntimeInstance = NewObject<URockItemInstance>(OwningInventory);
		if (ItemStack.RuntimeInstance)
		{
			ItemStack.RuntimeInstance->OwningInventory = OwningInventory;
		}
		else
		{
			UE_LOG(LogRockInventory, Error, TEXT("Failed to create runtime instance for item stack %s"), *ItemStack.GetDebugString());
		}
	}
	return ItemStack;
}

bool URockItemStackLibrary::CanStackWith(const FRockItemStack& FirstItem, const FRockItemStack& SecondItem)
{
	if (!FirstItem.IsValid() || !SecondItem.IsValid())
	{
		return false;
	}

	if (!FirstItem.CanStackWith(SecondItem))
	{
		return false;
	}
	// Check if we have room to stack
	const int32 MaxStackSize = GetMaxStackSize(FirstItem);
	if (MaxStackSize <= 0)
	{
		return false;
	}
	return (FirstItem.StackSize + SecondItem.StackSize) <= MaxStackSize;
}

int32 URockItemStackLibrary::GetMaxStackSize(const FRockItemStack& ItemStack)
{
	if (const URockItemDefinition* Def = GetItemDefinition(ItemStack.GetItemId()))
	{
		return Def->MaxStackSize;
	}
	return DEFAULT_MAX_STACK_SIZE;
}

bool URockItemStackLibrary::IsFull(const FRockItemStack& ItemStack)
{
	return ItemStack.StackSize >= GetMaxStackSize(ItemStack);
}
