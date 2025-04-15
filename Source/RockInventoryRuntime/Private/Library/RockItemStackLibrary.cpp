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
	if (const URockItemRegistrySubsystem* ItemRegistry = URockItemRegistrySubsystem::GetInstance())
	{
		if (const URockItemDefinition* Item = ItemRegistry->FindDefinition(ItemStack.ItemId))
		{
			return Item->SlotDimensions;
		}
	}
	return FVector2D(0, 0);
}

FRockItemStack URockItemStackLibrary::CreateItemStack(const FRockItemStack& InItemStack)
{
	FRockItemStack ItemStack = InItemStack;
	ItemStack.StackSize = 1; // Reset stack size to 1 for the new item stack
	ItemStack.RuntimeInstance = nullptr; // Reset runtime instance for the new item stack

	// If we have partial ItemStack, attempt to fill it out
	// if (ItemStack.ItemId != NAME_None && !ItemDefinition)
	// {
	// 	// TODO: Build out a proper ItemRegistry
	// 	// This would be a good time to LoadAsync certain aspects of the ItemDefinition
	//
	// 	// If we have an ItemId but no definition, we should look it up in the registry
	// 	// Definition = URockItemRegistry::Get()->FindDefinition(ItemId);
	// 	checkf(ItemDefinition, TEXT("ItemStack %s has no definition set!"), *ItemStack.GetDebugString());
	// }
	//else if (ItemStack.ItemId == NAME_None && !ItemStack.Definition)
	{
	//	UE_LOG(LogTemp, Error, TEXT("ItemStack %s has no ItemId or Definition set!"), *ItemStack.GetDebugString());
	//	return ItemStack;
	}

	const URockItemDefinition* ItemDefinition = URockItemStackLibrary::GetItemDefinition(ItemStack.ItemId);
	if (ItemDefinition && ItemDefinition->bRequiresRuntimeInstance)
	{
		ItemStack.RuntimeInstance = NewObject<URockItemInstance>(ItemDefinition->GetClass());
		if (ItemStack.RuntimeInstance == nullptr)
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
