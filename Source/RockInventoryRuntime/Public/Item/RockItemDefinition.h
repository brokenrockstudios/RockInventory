// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockItemFragment.h"
#include "Engine/DataAsset.h"
#include "RockItemDefinition.generated.h"

class UGameplayAbility;
class URockInventoryConfig;

USTRUCT(BlueprintType)
struct FRockItemUIData
{
	GENERATED_BODY()

public:
	// Hover text for the icon.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI", meta = (AssetBundles= "UI"))
	TSoftObjectPtr<UTexture2D> Icon;
};


USTRUCT()
struct FRockEquipmentActorToSpawn
{
	GENERATED_BODY()

	FRockEquipmentActorToSpawn()
	{
	}

	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
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
	// "RedApple"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId;
	//////////////////////////////////////////////////////////////////////////
	// Display (Tooltips)
	// e.g. "Apple"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Name;
	// "A delicious red apple."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText DisplayName;
	// "A crisp apple, perfect for a quick snack. Restores a small amount of health.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Description;
	//////////////////////////////////////////////////////////////////////////
	// Core (Inventory)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	int32 MaxStackSize = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	FVector2D SlotDimensions = FVector2D(1.0f, 1.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	FRockItemUIData IconData;
	//////////////////////////////////////////////////////////////////////////
	/// Information
	// An item can be 'multiple types' (e.g. a 'battery' can be a 'tool' and a 'battery')
	// Item.Type.Weapon, Item.Type.Tool, Item.Type.Consumable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTagContainer ItemType;
	// Though this could be a subtag of ItemType, it is separated to allow for more flexibility.
	// Item.SubType.Sword? Feels redundant to above? Perhaps have better usage?  
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag ItemSubType;
	// Item rarity, e.g. Item.Rarity.Common, Item.Rarity.Uncommon, Item.Rarity.Rare, Item.Rarity.Epic, Item.Rarity.Legendary
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTag ItemRarity;
	// General purpose tags that can be used for filtering, sorting, or categorization.
	// Item.Tag.Quality.Magical, Item.Tag.Elemental.Fire, Item.Tag.Set.DragonSlayer, Item.Tag.Binding.SoulBound, Item.Tag.Material.Steel
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	FGameplayTagContainer ItemTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	float Weight = 0.0f;
	// Not necessarily a 'price', but perhaps some internal value for sorting, crafting, or other purposes.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	float ItemValue = 0.0f;
	//////////////////////////////////////////////////////////////////////////
	/// World
	// If you need 'multiple', consider using Fragments instead
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<UStaticMesh> ItemMesh;
	// Always prefer SM over Skeletal, but allow for both.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<USkeletalMesh> ItemSkeletalMesh;

	// When this item is spawned in the world, it will spawn an actor of this class.
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
	// If this item requires a runtime instance, this is the class that will be used to create it.
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	TSubclassOf<class URockItemInstance> RuntimeInstanceClass;
	
	// Runtime Instances nested inventory.
	// e.g. If this Item was a Backpack, this should be set to the Backpack's InventoryConfig.
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragments", meta=(DisplayPriority = 100))
	TArray<FRockItemFragmentInstance> ItemFragments;


	// the equivalent to the 'right click menu'
	// Actions that all items can do
	// Drop / Split (Is there a scenario where we'd want an item to not be droppable or not be splittable?

	// Generally this should be "Use" but perhaps it's "Consume" or other things?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	FText UseItemTextOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	TSubclassOf<UGameplayAbility> UseItemAbility;
	// bActivateOnGranted = true;
	// InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;
	// bAutoRemoveOnEnd = true;

	// UPROPERTY(EditDefaultsOnly, Category="Usage")
	// EItemUseType UseType = EItemUseType::Immediate; // Immediate, Toggle?


	// GA_ConsumeApple: Heal 10 HP.
	// Is there a way we can generalize this?  Should GA_ConsumeApple be different then GA_ConsumePie
	// Or perhaps we can leverage some 'attribute' that is referenced from a GA_ConsumeGeneric, like leveraging the OnEquipment AbilityInfo?
	// How do we tell the ability that THIS item is the one that is being used? Can we pass it in with the context?

	// GA_UnwrapPresent: Spawn item in inventory or world.
	// GA_LearnSkillFireball: Add ability to player.

	// Perhaps instead of a direct ability, we could fall back to some 'generic' ability or global 'function library' based upon this tag?
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	//FGameplayTag UseAbilityTag; // Optional fallback route


	// This might want to be a singular GameplayAbility, slightly different from Equip.
	// Note: In the case of a 'hotbar', depending upon which slot is selected, that system likely relies upon Equip/Unequip
	// The hot bar will need to determine if we are attemping to 'use' the item, or 'equip' the item.
	// Because you are 'equipping' the gun, and then relying on Primary+Secondary triggers on it. 
	// Grenade is likely a 'equip'.

	// ConsumeItem (Should this be equivalent to UseItem?


	// OpenItem
	// Is this inventory inherit, and part of the 'runtime instance', since this would likely involve opening a nested inventory
	// In the case of like a 'wrapped gift', that should rely on UseItem instead.   

	// InspectItem
	// Is this Inventory inherit, or do we want some items to have special things. 

	// EquipItem
	// Apply gameplaytags, abilities, attributes, gameplayeffects on 'equip', for 'equipment

	// UnequipItem
	// Same as equip but reverse. 

	// UnloadItem
	// Not neccesarily only for 'guns', could also be like removing a battery


	// Destroy

	// destroy, drop, equip, inspect, open, unequip, unload, use

	// EquipmentFragment
	// Supports Attachment
	// CoreItems Tags required to function (e.g. a battery or a barrel)
	// SkelMesh
	// StaticMesh
	// EquipSocket?

	// TOOD: Should this be in a game specific fragment instead?
	// UPROPERTY(EditDefaultsOnly, Category=Equipment)
	// TSubclassOf<URockEquipmentInstance> InstanceType;

	// TOOD: Should this be in a game specific fragment instead?
	// Gameplay ability sets to grant when this is equipped
	// UPROPERTY(EditDefaultsOnly, Category=Equipment)
	// TArray<TObjectPtr<const URockItemAbilitySet>> AbilitySetsToGrant;

	// TOOD: Should this be in a game specific fragment instead?
	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FRockEquipmentActorToSpawn> ActorsToSpawn;

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
