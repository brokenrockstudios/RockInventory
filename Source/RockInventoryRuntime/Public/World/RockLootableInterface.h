// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "UObject/Interface.h"
#include "RockLootableInterface.generated.h"

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class URockLootableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ROCKINVENTORYRUNTIME_API IRockLootableInterface
{
	GENERATED_BODY()

public:
	/** Sets the internal item stack for this pickup item (e.g., after spawning or merging). */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	virtual void SetItemStack(const FRockItemStack& InItemStack) = 0;
	/**
	 * Returns the item this actor *offers* to the instigator for looting.
	 * May vary depending on team, pawn type, equipment, or internal logic.
	 * If InstigatorPawn is null, it is not context-sensitive.
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	virtual FRockItemStack GetItemStack(AActor* InstigatorPawn = nullptr) const = 0;

	// If using the transaction system, this might not be called. 
	/** Called whenever this item is picked up (client or server; possibly cosmetic). */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	virtual void OnPickedUp(AActor* Instigator) = 0;

	/**
	 * Called after a successful server-authoritative loot transaction.
	 * Implementations should update internal state (e.g., reduce stack count or destroy self).
	 */
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	virtual void OnLooted(AActor* InstigatorPawn, const FRockItemStack& LootedItem, int32 ExcessAmount) = 0;
};
