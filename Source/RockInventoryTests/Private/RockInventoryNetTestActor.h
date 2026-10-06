// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RockInventoryNetTestActor.generated.h"

class URockInventory;
class URockItemInstance;

/**
 * Replicated actor for the PIE network spike (T-68). It carries no replicated properties itself: its inventory and item
 * instances are registered subobjects whose replication the test gates with COND_NetGroup.
 * The server-side pointers only keep the objects alive; clients find their copies through the actor's inner objects.
 */
UCLASS(NotPlaceable)
class ARockInventoryNetTestActor : public AActor
{
	GENERATED_BODY()

public:
	ARockInventoryNetTestActor();

	UPROPERTY()
	TObjectPtr<URockInventory> ServerInventory;

	UPROPERTY()
	TArray<TObjectPtr<URockItemInstance>> ServerInstances;

	/** The inventory subobject this actor holds on this machine (the server's, or a client's replicated copy), or null. */
	URockInventory* FindInventory() const;

	/** The item instances held on this machine, in no particular order. */
	TArray<URockItemInstance*> FindInstances() const;
};
