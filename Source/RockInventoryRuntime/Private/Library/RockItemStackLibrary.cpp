// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockItemStackLibrary.h"

#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"

URockItemDefinition* URockItemStackLibrary::GetItemDefinitionById(const FName& ItemId)
{
	if (const URockItemRegistrySubsystem* ItemRegistry = URockItemRegistrySubsystem::GetInstance())
	{
		return ItemRegistry->FindDefinition(ItemId);
	}
	return nullptr;
}

URockItemDefinition* URockItemStackLibrary::GetItemDefinition(const FRockItemStack& ItemStack)
{
	return ItemStack.GetDefinition();
}

URockItemInstance* URockItemStackLibrary::GetRuntimeInstance(const FRockItemStack& ItemStack)
{
	if (ItemStack.IsValid())
	{
		return ItemStack.GetRuntimeInstance();
	}
	return nullptr;
}

int32 URockItemStackLibrary::GetStackSize(const FRockItemStack& ItemStack)
{
	return ItemStack.StackSize;
}

FVector2D URockItemStackLibrary::GetItemSize(const FRockItemStack& ItemStack)
{
	// We shouldn't have ItemStacks without valid definitions
	checkf(ItemStack.IsValid(), TEXT("ItemStack is invalid!"));
	if (ItemStack.IsValid())
	{
		const FVector2D Size = ItemStack.GetDefinition()->SlotDimensions;
		checkf(Size.X > 0 && Size.Y > 0, TEXT("ItemStack %s has invalid size!"), *ItemStack.GetDebugString());
		return Size;
	}
	return FVector2D(1, 1);
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
	return ItemStack.GetMaxStackSize();
}

bool URockItemStackLibrary::IsFull(const FRockItemStack& ItemStack)
{
	return ItemStack.StackSize >= GetMaxStackSize(ItemStack);
}
