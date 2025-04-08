// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockItemDefinitionFragment.h"
#include "Engine/DataAsset.h"
#include "RockItemDefinition.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class ROCKINVENTORYRUNTIME_API URockItemDefinition : public UPrimaryDataAsset
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
	TSoftObjectPtr<UStaticMesh> ItemMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UStaticMeshComponent> ItemMesh2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TWeakObjectPtr<UStaticMeshComponent> ItemMesh3;
	//TSoftClassPtr<UStaticMesh> ItemMesh;  // NOT this one
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

	TArray<FRockItemDefinitionFragmentInstance > ItemComponents;

	// EquipmentFragment
	// Supports Attachment
	// CoreItems Tags required to function (e.g. a battery or a barrel)
	// SkelMesh
	// StaticMesh
	// EquipSocket?
	
	
};
