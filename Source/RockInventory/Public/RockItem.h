// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockItem.generated.h"
//test

class URockItemInstance;
class URockItemDefinition;

// a stack of items
USTRUCT(BlueprintType)
struct ROCKINVENTORY_API FRockItemStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	// ID to look up the definition in your registry
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StackSize = 1;

	// CustomValue is a generic value that can be used for anything. To avoid needing a Runtime Instance
	// e.g. Durability, ChargeCount, Seed for RNG, Bitmask, progress, timer reference, variant id, or level
	// If you want or need more than one of these, you should probably use a runtime instance or modify for a 2nd CustomValue. 
	// Generally I imagine most of the time it should just be Durability, ChargeCount, or something like that.
	// though we probably don't need the full int32, so consider masking it to a smaller value. 
	UPROPERTY()
	int32 CustomValue1 = 0;
	// We might add a second custom value if we feel there is a justifable reason for it instead of the RuntimeInstance
	// Otherwise want to try and keep this as simple as possible.

	UPROPERTY()
	TObjectPtr<URockItemInstance> RuntimeInstance = nullptr;

	/** Resolve and cache the item definition */
	const URockItemDefinition* GetDefinition() const;
	UPROPERTY(Transient)
	TObjectPtr<URockItemDefinition> CachedDefinition = nullptr;
};

// FRockItemStackContainer
USTRUCT()
struct ROCKINVENTORY_API FRockItemStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()
public:
	// holds an array of items for a given container

	// SlotID?
	// index or x/y/rot? (tab?)
	
};


UENUM(BlueprintType)
enum class ERockItemRotation : uint8
{
	Horizontal = 0,
	Vertical = 1
};

// SpecialSlot types? Only consider special designated slots for hotswap


// Base class for all item instances
UCLASS(BlueprintType)
class ROCKINVENTORY_API URockItemInstance : public UObject
{
	GENERATED_BODY()

public:
	// We probably want a pointer to the actual struct instead, otherwise it's a copy.
	// UPROPERTY() ? 
	TSharedPtr<FRockItemStack> ItemBase = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Item")
	const URockItemDefinition* GetItemDefinition() const;
	
	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~End of UObject interface

	// FGameplayTagStackContainer StatTags;
	
	// TODO: Should all ItemInstances have nested inventories?
	// TODO: Should all ItemInstances have Fragments?	
};

inline const URockItemDefinition* URockItemInstance::GetItemDefinition() const
{
	if (ensureMsgf(ItemBase, TEXT("ItemBase is null!")))
	{
		return ItemBase->GetDefinition();
	}
	return nullptr;
}

UCLASS(Blueprintable)
class URockItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId;
	// Do we want a unique FGuid for any purpose? e.g. UniqueServerID
	// Might be useful beyond ItemID?

	// Short name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Name;
	
	// Long name
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FVector2D SlotDimensions;
		

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag ItemType; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag ItemSubType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag ItemRarity;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	float Weight = 0.0f;

	// Not necessarily a price per-se, but a value for the item.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    float ItemValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftClassPtr<UStaticMesh> ItemMesh;
	// Always prefer SM over Skeletal, but allow for both.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftClassPtr<USkeletalMesh> ItemSkeletalMesh;
	
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSoftClassPtr<AActor> ActorClass;

	// Used to identify the purpose or functionality of the item's CustomValue1.
	// Is it used for durability, charge, etc..
	// Build a system to use this to determine what the value is used for.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag CustomValue1Tag;
	// Durability, charges, etc..

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditDefaultsOnly)
	bool bRequiresRuntimeInstance = false;

	// Should we just remove this in favor of RuntimeInstance?
	UPROPERTY(EditDefaultsOnly)
	bool bCanContainItems = false;

	// drag/drop sound for UI?
	
	//UPROPERTY(EditDefaultsOnly)
	//int32 MaxContainedItems = 24; // optional grid/weight limit
	// bool IsContainer = false;
	// bool IsStackable = false;

	// Add Fragments for things regarding 3D model or sounds?
	
	// Consider adding getter functions for common things like
	// Usable
	// Equipable

	// destroy, drop, equip, inspect, open, unequip, unload, use
	
	// Consumable Fragment "Consume Item"
	// GA to occur on consume or GameplayEffect?
	// On consume destroy item or consume charges or something?
	 
	// UsableFragment "Use" item
	// GA to occur on use or GameplayEffect?


	// Directly access if they are valid!
	// These may be 'null', depending if the item supports it.
	//UPROPERTY()
	//TObjectPtr<URockEquipmentFragment> EquipmentFragment = nullptr;
	//UPROPERTY()
	//TObjectPtr<UConsumableEquipmentFragment> EquipmentFragment = nullptr;
	
	
	// Only use ItemFrags array for if you have some special cases where you want a non traditional fragment or 1-off?	
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	//TArray<TSubclassOf<UObject>> ItemFragments;

	

	// EquipmentFragment
	// Supports Attachment
	// CoreItems Tags required to function (e.g. a battery or a barrel)
	// SkelMesh
	// StaticMesh
	// EquipSocket?
	
	
};
