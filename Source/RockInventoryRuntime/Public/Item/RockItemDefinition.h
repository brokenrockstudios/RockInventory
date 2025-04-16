// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockItemDefinitionFragment.h"
#include "Engine/AssetManager.h"
#include "Engine/DataAsset.h"
#include "RockItemDefinition.generated.h"

class URockInventoryConfig;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//////////////////////////////////////////////////////////////////////////	
	// Item ID
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId;
	// Do we want a unique FGuid for any purpose? e.g. UniqueServerID
	// Might be useful beyond ItemID?

	//////////////////////////////////////////////////////////////////////////
	// Display (Tooltips)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Name;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Description;

	//////////////////////////////////////////////////////////////////////////
	// Core (Inventory)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	int32 MaxStackSize = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	FVector2D SlotDimensions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	TSoftObjectPtr<UTexture2D> Icon;


	//////////////////////////////////////////////////////////////////////////
	/// Information
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag ItemType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag ItemSubType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag ItemRarity;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	float Weight = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	float ItemValue = 0.0f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTagContainer ItemTags;


	//////////////////////////////////////////////////////////////////////////
	/// World
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<UStaticMesh> ItemMesh;
	// Always prefer SM over Skeletal, but allow for both.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<USkeletalMesh> ItemSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Item|World")
	TSoftClassPtr<AActor> ActorClass;


	//////////////////////////////////////////////////////////////////////////
	/// Advanced
	// Used to identify the purpose or functionality of the item's CustomValue1.
	// Is it used for durability, charge, etc..
	// Build a system to use this to determine what the value is used for.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag CustomValue1Tag;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	bool bRequiresRuntimeInstance = false;

	// Runtime Instances nested inventory.
	// e.g. If this Item was a Backpack, this should be set
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	TSoftObjectPtr<URockInventoryConfig> InventoryConfig = nullptr;


	//////////////////////////////////////////////////////////////////////////
	/// Misc
	UPROPERTY(EditDefaultsOnly, Category = "Item|Misc")
	TSoftObjectPtr<USoundBase> PickupSoundOverride = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Item|Misc")
	TSoftObjectPtr<USoundBase> DropSoundOverride = nullptr;



	
	// Add Fragments for things regarding 3D model or sounds?
	// Consider adding getter functions for common things like
	// Usable
	// Equippable

	// destroy, drop, equip, inspect, open, unequip, unload, use

	// Consumable Fragment "Consume Item"
	// GA to occur on consume or GameplayEffect?
	// On consume destroy item or consume charges or something?

	// UsableFragment "Use" item
	// GA to occur on use or GameplayEffect?

	// Directly access if they are valid!
	// These may be 'null', depending on if the item supports it.
	//UPROPERTY()
	//TObjectPtr<URockEquipmentFragment> EquipmentFragment = nullptr;
	//UPROPERTY()
	//TObjectPtr<UConsumableEquipmentFragment> EquipmentFragment = nullptr;


	// Only use ItemFrags array for if you have some special cases where you want a non traditional fragment or 1-off?	
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	//TArray<TSubclassOf<UObject>> ItemFragments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TArray<FRockItemDefinitionFragmentInstance> ItemComponents;

	// EquipmentFragment
	// Supports Attachment
	// CoreItems Tags required to function (e.g. a battery or a barrel)
	// SkelMesh
	// StaticMesh
	// EquipSocket?


	// If creating a 'runtime instance definition', we'd need to manually register it with the asset manager
	// e.g. 
	
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		if (ItemId != NAME_None)
		{
			return FPrimaryAssetId("RockItemDefinition", ItemId);
		}
		return FPrimaryAssetId("RockItemDefinition", GetFName());
	}


	// Experimental 
	void RegisterItemDefinition(const URockItemDefinition* NewItem);
};
