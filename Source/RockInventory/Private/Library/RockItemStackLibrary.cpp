// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockItemStackLibrary.h"

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
	if (!World) {
		//UE_LOG(LogRockItemRegistry, Warning, TEXT("FRockItemStack::GetDefinition could not get World from WorldContextObject."));
		return nullptr;
	}
	
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) {
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
