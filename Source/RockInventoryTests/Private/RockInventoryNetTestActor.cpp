// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryNetTestActor.h"

#include "Inventory/RockInventory.h"
#include "Item/RockItemInstance.h"
#include "UObject/UObjectHash.h"

ARockInventoryNetTestActor::ARockInventoryNetTestActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetNetUpdateFrequency(100.f);
	AActor::SetReplicateMovement(true);
}

URockInventory* ARockInventoryNetTestActor::FindInventory() const
{
	TArray<UObject*> Inner;
	GetObjectsWithOuter(this, Inner, EGetObjectsFlags::None);
	for (UObject* Object : Inner)
	{
		if (URockInventory* Inventory = Cast<URockInventory>(Object))
		{
			return Inventory;
		}
	}
	return nullptr;
}

TArray<URockInventory*> ARockInventoryNetTestActor::FindInventories() const
{
	TArray<URockInventory*> Result;
	TArray<UObject*> Inner;
	GetObjectsWithOuter(this, Inner, EGetObjectsFlags::None);
	for (UObject* Object : Inner)
	{
		if (URockInventory* Inventory = Cast<URockInventory>(Object))
		{
			Result.Add(Inventory);
		}
	}
	return Result;
}

TArray<URockItemInstance*> ARockInventoryNetTestActor::FindInstances() const
{
	TArray<URockItemInstance*> Result;
	TArray<UObject*> Inner;
	GetObjectsWithOuter(this, Inner, EGetObjectsFlags::IncludeNestedObjects);
	for (UObject* Object : Inner)
	{
		if (URockItemInstance* Instance = Cast<URockItemInstance>(Object))
		{
			Result.Add(Instance);
		}
	}
	return Result;
}
