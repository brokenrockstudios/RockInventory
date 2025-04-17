// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockItemFragment.h"
#include "Engine/DataAsset.h"
#include "RockItemDefinition.generated.h"

class URockInventoryConfig;

USTRUCT(BlueprintType)
struct FRockItemUIData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<UTexture2D> Icon;
};


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
	FRockItemUIData IconData;
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
	// Could/Should this be an enum instead?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag CustomValue1Tag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag CustomValue2Tag;
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	bool bRequiresRuntimeInstance = false;
	// Runtime Instances nested inventory.
	// e.g. If this Item was a Backpack, this should be set
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	TSoftObjectPtr<URockInventoryConfig> InventoryConfig = nullptr;
	//////////////////////////////////////////////////////////////////////////
	/// Sound
	UPROPERTY(EditDefaultsOnly, Category = "Item|Misc", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<USoundBase> InventoryMoveSoundOverride = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Item|Misc", meta = (AssetBundles= "Gameplay"))
	TSoftObjectPtr<USoundBase> PickupSoundOverride = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Item|Misc", meta = (AssetBundles= "Gameplay"))
	TSoftObjectPtr<USoundBase> DropSoundOverride = nullptr;

	///////////////////////////////////////////////////////////////////////////
	/// Fragments
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TArray<FRockItemFragmentInstance> ItemFragments;


	// destroy, drop, equip, inspect, open, unequip, unload, use
	// Consumable Fragment "Consume Item"
	// GA to occur on consume or GameplayEffect?
	// On consume destroy item or consume charges or something?
	// UsableFragment "Use" item
	// GA to occur on use or GameplayEffect?
	// These may be 'null', depending on if the item supports it.

	// EquipmentFragment
	// Supports Attachment
	// CoreItems Tags required to function (e.g. a battery or a barrel)
	// SkelMesh
	// StaticMesh
	// EquipSocket?

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		if (ItemId != NAME_None)
		{
			return FPrimaryAssetId("RockItemDefinition", ItemId);
		}
		return FPrimaryAssetId("RockItemDefinition", GetFName());
	}


	// If creating a 'runtime instance definition', we'd need to manually register it with the asset manager
	// e.g. 
	// Experimental 
	void RegisterItemDefinition(const URockItemDefinition* NewItem);
};
