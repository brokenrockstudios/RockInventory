// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagStack.h"
#include "Item/RockItemState.h"

#include "RockItemState_Metadata.generated.h"

/**
 * Generic runtime metadata state: loose gameplay tags and stacked stat tags for an item instance.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockItemState_Metadata : public FRockItemState
{
	GENERATED_BODY()
	
protected:
	/** Loose gameplay tags associated with this item instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	FGameplayTagContainer Tags;
	
	/** Stat tags associated with this item instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RockInventory|Stats")
	FGameplayTagStackContainer StatTags;
	
public:
	
	virtual void OnStateAdded(URockItemInstance* OwnerInstance) override;
	virtual void OnStateRemoved(URockItemInstance* OwnerInstance) override;
	
	// Tags
	bool HasTag(FGameplayTag Tag) const;
	void AddTag(FGameplayTag Tag);
	void RemoveTag(FGameplayTag Tag);
	
	// StatTags
	int32 GetStatTagCount(FGameplayTag Tag) const;
	void AddStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks = false);
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks = false);
	void SetStatTagCount(FGameplayTag Tag, int32 StackCount, bool bKeepZeroStacks = false);
	bool ContainsTag(FGameplayTag Tag) const;
};
