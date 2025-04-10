// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockItemStackLibrary.h"

#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Item/ItemRegistry/RockItemRegistry.h"

URockItemDefinition* URockItemStackLibrary::GetItemDefinition(const UObject* WorldContextObject, const FName& ItemId)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	if (ItemId.IsNone())
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		//UE_LOG(LogRockItemRegistry, Warning, TEXT("FRockItemStack::GetDefinition could not get World from WorldContextObject."));
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		//UE_LOG(LogRockItemRegistry, Warning, TEXT("FRockItemStack::GetDefinition could not get GameInstance from World."));
		return nullptr;
	}

	if (const URockItemRegistry* Registry = GameInstance->GetSubsystem<URockItemRegistry>())
	{
		URockItemDefinition* FoundDef = Registry->FindDefinition(ItemId); // Use the member ItemId

		// Use const_cast ONLY for caching transient data derived from replicated state (or data loaded from disk).
		// This is generally acceptable as CachedDefinition is marked Transient and is just an optimization.
		// Ensure CachedDefinition is appropriately cleared if ItemId changes or the item stack is invalidated.
		//const_cast<FRockItemStack*>(this)->CachedDefinition = FoundDef;

		return FoundDef;
	}
	else
	{
		//UE_LOG(LogRockItemRegistry, Error, TEXT("FRockItemStack::GetDefinition failed to get URockItemRegistry subsystem!"));
	}

	return nullptr;
}

FVector2D URockItemStackLibrary::GetItemSize(const FRockItemStack& ItemStack)
{
	if (ItemStack.GetDefinition())
	{
		return ItemStack.GetDefinition()->SlotDimensions;
	}
	return FVector2D(0, 0);
}

FRockItemStack URockItemStackLibrary::CreateItemStack(const FRockItemStack& InItemStack)
{
	FRockItemStack ItemStack = InItemStack;
	ItemStack.StackSize = 1; // Reset stack size to 1 for the new item stack
	ItemStack.RuntimeInstance = nullptr; // Reset runtime instance for the new item stack
	
	// If we have partial ItemStack, attempt to fill it out
	if (ItemStack.ItemId == NAME_None && ItemStack.Definition)
	{
		ItemStack.ItemId = ItemStack.Definition->ItemId;
	}
	else if (ItemStack.ItemId != NAME_None && !ItemStack.Definition)
	{
		// TODO: Build out a proper ItemRegistry
		// This would be a good time to LoadAsync certain aspects of the ItemDefinition
		
		// If we have an ItemId but no definition, we should look it up in the registry
		// Definition = URockItemRegistry::Get()->FindDefinition(ItemId);
		checkf(ItemStack.Definition, TEXT("ItemStack %s has no definition set!"), *ItemStack.GetDebugString());
	}
	else if (ItemStack.ItemId == NAME_None && !ItemStack.Definition)
	{
		UE_LOG(LogTemp, Error, TEXT("ItemStack %s has no ItemId or Definition set!"), *ItemStack.GetDebugString());
		return ItemStack;
	}

	if (ItemStack.GetDefinition()->bRequiresRuntimeInstance)
	{
		ItemStack.RuntimeInstance = NewObject<URockItemInstance>(ItemStack.GetDefinition()->GetClass());
	} 
	return ItemStack;
}
