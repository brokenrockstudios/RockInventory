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
	return ItemStack.StackCount;
}

FIntPoint URockItemStackLibrary::GetItemSize(const FRockItemStack& ItemStack)
{
	// We shouldn't have ItemStacks without valid definitions
	ensureMsgf(ItemStack.IsValid(), TEXT("ItemStack is invalid!"));
	if (ItemStack.IsValid())
	{
		// TODO: Should we consider the size of the runtime instance?
		// e.g. A weapon with/without a suppressor might have different sizes.
		
		const FIntPoint Size = ItemStack.GetDefinition()->GridSize;
		checkf(Size.X > 0 && Size.Y > 0, TEXT("ItemStack %s has invalid size!"), *ItemStack.GetDebugString());
		return Size;
	}
	return FIntPoint(1, 1);
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
	return (FirstItem.GetStackCount() + SecondItem.GetStackCount()) <= MaxStackSize;
}

int32 URockItemStackLibrary::GetMaxStackSize(const FRockItemStack& ItemStack)
{
	return ItemStack.GetMaxStackSize();
}

bool URockItemStackLibrary::IsFull(const FRockItemStack& ItemStack)
{
	return ItemStack.GetStackCount() >= GetMaxStackSize(ItemStack);
}

int32 URockItemStackLibrary::CalculateMoveAmount(const FRockItemStack& ItemStack, const ERockItemMoveMode& MoveMode, int32 MoveCount)
{
	if (!ItemStack.IsValid())
	{
		return 0;
	}

	const int32 CurrentStackSize = ItemStack.GetStackCount();
	if (CurrentStackSize <= 0)
	{
		return 0;
	}

	switch (MoveMode)
	{
	case ERockItemMoveMode::FullStack:
		return CurrentStackSize;
	case ERockItemMoveMode::SingleItem:
		return 1;
	case ERockItemMoveMode::HalfStack:
		// Rounded up: (5 → 3), (1 → 1), (4 → 2)
		return (CurrentStackSize + 1) / 2;
	case ERockItemMoveMode::CustomAmount:
		// Clamp the custom amount to the current stack size
		return FMath::Clamp(MoveCount, 0, CurrentStackSize);
	default:
		// Invalid move amount type, return 0
		return 0;
	}
}
