// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemState.generated.h"

class URockItemInstance;

/**
 * Base struct for replicated runtime state data attached to an item instance.
 * Mirrors FRockItemFragment, but represents mutable runtime state (e.g. current durability)
 * rather than static definition data (e.g. max durability).
 */
USTRUCT(BlueprintType, meta=(DisplayName="Item State"))
struct ROCKINVENTORYRUNTIME_API FRockItemState
{
	GENERATED_BODY()
	FRockItemState() = default;
	
	UPROPERTY()
	TWeakObjectPtr<URockItemInstance> OwningInstance = nullptr;

	// Rule of five: Because the presence of a user-defined destructor should declare all five special member functions
	virtual ~FRockItemState() = default;
	// Copy
	FRockItemState(const FRockItemState&) = default;
	FRockItemState& operator=(const FRockItemState&) = default;
	// Move
	FRockItemState(FRockItemState&&) = default;
	FRockItemState& operator=(FRockItemState&&) = default;

	// ~Begin Lifecycle events for the state. Called by the owning ItemInstance at appropriate times.
	virtual void OnStateAdded(URockItemInstance* OwnerInstance);
	virtual void OnStateRemoved(URockItemInstance* OwnerInstance);
	void NotifyChanged();
	// ~End Lifecycle events
};


// Example of a state.
// We could leverage the OwningInstance to get the ItemDefinition's fragment data that might contain Max Durability, or we could store off a copy here too. 
// bool FRockItemState_Durability::SetDurability(int32 NewValue)
// {
// 	NewValue = FMath::Clamp(NewValue, 0, MaxDurability);
//
// 	if (Durability == NewValue)
// 	{
// 		return false;
// 	}
//
// 	Durability = NewValue;
// 	NotifyChanged();
// 	return true;
// }
