// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagStack.h"
#include "RockItemFragment.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
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

	// TBD
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI", meta = (AssetBundles= "UI"))
	// FVector2D IconSize = FVector2D(44.0f, 44.0f);
};

/**
 * Note: No Derived Blueprint classes and overriden values don't always work. Might need to re-force some indexing. But still might not work.
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	//////////////////////////////////////////////////////////////////////////    
	// Item ID
	// The internal ItemID. e.g. /spawn RedApple. This is not the DisplayName or Name.
	// We technically could have used the AssetName as the ItemID, but this allows for more flexibility and control.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId;
	//////////////////////////////////////////////////////////////////////////
	// Display (Tooltips) 
	// A short name, such as "Fuel" as opposed to the longer Display name of "Gasoline Fuel Canister"
	// e.g. "Apple"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Name;
	// "A delicious red apple." as opposed to a short name(Name) such as "Apple"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText DisplayName;
	// "A crisp apple, perfect for a quick snack. Restores a small amount of health."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Display")
	FText Description;
	//////////////////////////////////////////////////////////////////////////
	// Core (Inventory)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	int32 MaxStackCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	FIntPoint GridSize = FIntPoint(1, 1);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	FRockItemUIData IconData;
	//////////////////////////////////////////////////////////////////////////
	/// Information
	// Examples
	// "M4 Rifle"           | ItemType { Item.Type.Weapon, Item.Type.Ranged }                      Tags { Tag.Equippable, Tag.TwoHanded, Tag.Automatic, Tag.UsesAmmo.Rifle.556, Tag.Attachable, Tag.Suppressable }
	// "A wooden shield"    | ItemType { Item.Type.Armor, Item.Type.Offhand, Item.Type.Shield }    Tags { Tag.Equippable, Tag.Blocking, Tag.Wooden, Tag.OneHanded }
	// "Aviator Sunglasses" | ItemType { Item.Type.Armor, Item.Type.Accessory }                    Tags { Tag.Equippable, Tag.Slot.Face, Tag.Slot.Eyes }
	// "Baseball Cap"       | ItemType { Item.Type.Cosmetic }                                      Tags { Tag.Equippable, Tag.Slot.Head }
	// "Leather Gloves"     | ItemType { Item.Type.Armor, Item.Type.Cosmetic }                     Tags { Tag.Equippable, Tag.Slot.Hands }
	// "9mm Magazine"       | ItemType { Item.Type.Ammo, Item.Type.Consumable }                    Tags { Tag.Ammo.Caliber.9mm, Tag.Stackable, Tag.FitsWeapon.Pistol }
	// "Cooked Meat"        | ItemType { Item.Type.Consumable, Item.Type.Food }                    Tags { Tag.HealsHP, Tag.Cooked, Tag.CanSpoil }
	// "Scroll of Fireball" | ItemType { Item.Type.Consumable, Item.Type.Magic, Item.Type.Scroll } Tags { Tag.Property.SingleUse, Tag.Effect.Damage.Fire.AreaOfEffect, Tag.Character.Class.CanBeUsedBy.Mage }

	// Vendor: I only sell Item.Type.Weapon, I only Buy Item.Type.Food
	// AI looting: Prioritize Ammo and Weapons
	// Quest requests 2 Item.Type.Ore (instead of providing a list of all 'ores')
	// Slot can hold ANY(Item.Type.Weapon, Item.Type.Shield, Item.Type.Consumable)
	// Slot can hold ALL(Item.Type.Armor AND NOT Item.Type.Cosmetic)
	// Slot requires ALL(Item.Type.Cosmetic AND Item.Tag.Slot.Eyes)
	// Slot requires ALL(Item.Type.Cosmetic AND Item.Tag.Slot.Torso.Outer)

	// Other potential ItemTag ideas
	// Tag.Binding.NoTrade, Tag.Binding.NoPortal, Tag.Binding.DeathDrop, Tag.Binding.ServerLocked etc. A system just checks "does this item have Tag.Binding.NoPortal
	// That aren't really fragment worthy but might be mostly boolean ish in nature
	// Alternative to Fragments 
	//  * Tag.Material.Wood,Metal,Steel instead of Fragment material
	//  * Tag.Elemental.Fire (should probably live in a Fragment_DamageModifier, damagetype?
	//  * TwoHanded vs. OneHanded

	// ItemType answers "what kind of thing is this?"
	// Tags answer "what can this thing do / where can it go?"

	// For SectionInfo SectionFilters

	// Wrong approach - slot filter on Subtype
	// SectionFilter: Item.Subtype.Weapon.Firearm.Rifle
	// Now you need a separate slot definition for every weapon subcategory

	// Right approach - slot filter on ItemType or behavioral tag
	// Primary weapon slot SectionFilter: Item.Type.Weapon && Item.Tag.TwoHanded
	// Offhand slot SectionFilter:        Item.Type.Offhand || (Item.Type.Weapon && Item.Tag.OneHanded)
	// Rifle-only slot SectionFilter:     Item.Type.Weapon && Item.Tag.Firearm.Rifle
	// Head slot SectionFilter:           Item.Type.Armor && Item.Tag.Slot.Head

	// Slot SectionFilters:
	// Hands slot:   Item.Type.Armor AND Tag.Slot.Hands                        -> accepts gloves, gauntlets, etc.
	// Head slot:    Item.Type.Armor AND Tag.Slot.Head                         -> accepts cap, helmet, etc.
	// Eyes slot:    Item.Type.Armor AND Tag.Slot.Eyes                         -> accepts only eyewear
	// Face slot:    (Item.Type.Armor OR Item.Type.Cosmetic) AND Tag.Slot.Face -> accepts glasses, mask, etc.

	// Note: In case of an 'animation' or needing an explicit type, consider a specific Item.Tag choice

	// An item can be 'multiple types' (e.g. a 'battery' can be a 'tool' and a 'battery')
	// Item.Type.Weapon, Item.Type.Tool, Item.Type.Consumable
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta=(GameplayTagFilter="ItemType"))
	FGameplayTagContainer ItemType;

	// General purpose tags that can be used for filtering, sorting, or categorization.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta=(GameplayTagFilter = "ItemTags"))
	// Item.Tag.Quality.Premium, Item.Tag.Elemental.Fire, Item.Tag.Slot.Face, Item.Tag.DestroyOnDeath, Item.Tag.Material.Steel
	FGameplayTagContainer ItemTags;

	// Item rarity, e.g. Item.Rarity.Common, Item.Rarity.Uncommon, Item.Rarity.Rare, Item.Rarity.Epic, Item.Rarity.Legendary
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta=(Categories="Item.Rarity"))
	FGameplayTag ItemRarity;

	// The user facing final value will likely be derived from a combination.
	// A lightweight item likely stacks and is multiplied by its stack count and may show decimals or not as see fit.
	// Int math > float math.
	// Unloaded M16     7.18 lbs  3.26 kg   3,260,000 mg
	// Steel Longsword   2.5 lbs   1.4 kg   1,400,000 mg
	// 1 Feather       .0002 lbs .0001 kg         100 mg
	// apple seed                                  10 mg
	// a single ant                                 2 mg
	// 1000 Feather       .2 lbs   0.1 kg       1,000 mg
	// 1 worm           .002 lbs  .001 kg         100 mg
	// 1 flower petal  .0002 lbs .0001 kg         100 mg
	// 100 kg dumbbell             100 kg 100,000,000 mg
	// mech suit                   500 kg 500,000,000 mg
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta=(ToolTip="Weight in milligrams. 1g = 1000, 1kg = 1,000,000, 1kg ~ 2.2lbs"))
	int64 Weight = 1000;

	// Not necessarily a 'price', but perhaps some internal value for sorting, crafting, or other purposes.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information")
	int32 ItemValue = 0;
private:
	// Note: These are for unchanging values. If you need dynamic values use the StatTags in RuntimeItemInstance.
	UPROPERTY(EditDefaultsOnly, Category = "Item|Stats", meta = (TitleProperty = "Tag", DisplayName = "Stat Tags"))
	TArray<FGameplayTagStack> StatTagDefaults;

	// A cache of ItemType, Tags, Subtype
	UPROPERTY(Transient) // not EditAnywhere, derived
	FGameplayTagContainer CachedAllTags;
public:
	// The main StatTags that should be queried. Readonly
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Stats", meta = (DisplayName = "Stat Tags (Runtime)"))
	FGameplayTagStackContainer StatTags;

	//////////////////////////////////////////////////////////////////////////
	/// World
	// If you need 'multiple', consider using Fragments instead
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<UStaticMesh> ItemMesh;
	// Always prefer SM over Skeletal, but allow for both.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<USkeletalMesh> ItemSkeletalMesh;

	// When this item is spawned in the world, it will spawn an actor of this class.
	// This is useful if you want to place a 'live version' of the item (e.g. a campfire you can interact with).
	// Though maybe you'd rather leverage this for something different and have a ItemFragment_Placeable or something
	UPROPERTY(EditDefaultsOnly, Category = "Item|World")
	TSoftClassPtr<AActor> ActorClass;

	//////////////////////////////////////////////////////////////////////////
	/// Advanced
	// Used to identify the purpose or functionality of the item's CustomValue1.
	// Is it used for durability, charge, etc..
	// Build a system to use this to determine what the value is used for.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta = (DisplayName = "Custom Value 1 Tag"))
	FGameplayTag CustomValue1Tag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Information", meta = (DisplayName = "Custom Value 2 Tag"))
	FGameplayTag CustomValue2Tag;

	// If this item requires a runtime instance, this is the class that will be used to create it.
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	TSoftClassPtr<class URockItemInstance> RuntimeInstanceClass;

	// Runtime Instances nested inventory.
	// e.g., If this Item was a Backpack, this should be set to the Backpack's InventoryConfig.
	UPROPERTY(EditDefaultsOnly, Category = "Item|Advanced")
	TSoftObjectPtr<URockInventoryConfig> InventoryConfig = nullptr;

	///////////////////////////////////////////////////////////////////////////
	/// Fragments

	/** Item Fragments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragments", meta=(DisplayPriority = 100, BaseStruct = "/Script/RockInventoryRuntime.RockItemFragment"))
	TArray<FInstancedStruct> Fragments;

	template <typename T> requires std::derived_from<T, FRockItemFragment>
	const T* FindFragment() const;

	template <typename T> requires std::derived_from<T, FRockItemFragment>
	bool HasFragment() const;

	const TArray<FInstancedStruct>& GetAllFragments() const;

	const FGameplayTagContainer& GetAllTags() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// If creating a 'runtime instance definition', we'd need to manually register it with the asset manager e.g. Experimental 
	void RegisterItemDefinition(const URockItemDefinition* NewItem);
	virtual UClass* GetWorldItemClass();
private:
	virtual void PostLoad() override;

	// If ItemID is not set, we can default it to the asset name. This is for ease of use, but it is still recommended to set it explicitly.
	void SetDefaultItemId();

	// This is used to rebuild the StatTags from the StatTagDefaults. This is necessary because the StatTags are meant to be runtime values, while the 
	// StatTagDefaults are meant to be design time values. This allows for the StatTags to be modified at runtime without affecting the defaults. This is also 
	// used to rebuild the StatTags when the item definition is loaded or when a property is changed in the editor. This ensures that the StatTags are always 
	// up to date with the StatTagDefaults. All of this is simply because FGameplayTagStackContainer doesn't allow setting default values in the constructor.
	// If it did, we could simply set the StatTags and avoid all of this. But since it doesn't, we need to manually copy the values over.
	void RebuildStatTags();
	void RebuildCachedTags();

	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

template <typename T> requires std::derived_from<T, FRockItemFragment>
const T* URockItemDefinition::FindFragment() const
{
	for (const FInstancedStruct& Fragment : GetAllFragments())
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
			return FragmentPtr;
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FRockItemFragment>
bool URockItemDefinition::HasFragment() const
{
	return FindFragment<T>() != nullptr;
}
