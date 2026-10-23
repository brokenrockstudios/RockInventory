// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RockAssetRef.h"
#include "StructUtils/InstancedStruct.h"

#include "RockItemRecord.generated.h"

namespace RockItemData
{
	/** The RockData type name of an item record. The record ID is the ItemId. */
	ROCKINVENTORYDATA_API extern const FName ItemTypeName;
}


/** The assets an item names. Each is a full path or an alias; all optional. */
USTRUCT()
struct FRockItemAssets
{
	GENERATED_BODY()

	UPROPERTY(meta = (RockAssetClass = "Texture2D"))
	FRockAssetRef Icon;

	UPROPERTY(meta = (RockAssetClass = "StaticMesh"))
	FRockAssetRef Mesh;

	UPROPERTY(meta = (RockAssetClass = "SkeletalMesh"))
	FRockAssetRef SkeletalMesh;

	/**
	 * The live, active, spawned version of the item: what it becomes when it is placed, deployed or thrown (a chair, a turret, a grenade
	 * actor). Not the dropped pickup (that is the world item class, and a future WorldItemOverride fragment would change it) and not the
	 * equipped weapon actor (Equippable fragment). Fills URockItemDefinition::ActorClass.
	 */
	UPROPERTY(meta = (RockAssetClass = "Actor", RockAssetAsClass = true))
	FRockAssetRef SpawnedActorClass;

	UPROPERTY(meta = (RockAssetClass = "/Script/RockInventoryRuntime.RockItemInstance", RockAssetAsClass = true))
	FRockAssetRef RuntimeInstanceClass;
};

/**
 * The core fields of URockItemDefinition as a RockData record (`Content/Data/Items/<...>/<ItemId>.jsonc`).
 * URockItemRecordProvider builds one URockItemDefinition per concrete record from it. Text (Name, DisplayName, Description) is not
 * a member: it comes from the text files by key `Item.<ID>.<Field>`.
 */
USTRUCT()
struct ROCKINVENTORYDATA_API FRockItemRecord
{
	GENERATED_BODY()

	UPROPERTY()
	int32 MaxStackCount = 1;

	/** Slots the item covers, `{ "X": 2, "Y": 1 }`. */
	UPROPERTY()
	FIntPoint GridSize = FIntPoint(1, 1);

	UPROPERTY()
	TArray<FGameplayTag> ItemType;

	UPROPERTY()
	TArray<FGameplayTag> ItemTags;

	UPROPERTY()
	FGameplayTag ItemRarity;

	/** Milligrams. */
	UPROPERTY()
	int64 Weight = 1000;

	UPROPERTY()
	int32 ItemValue = 0;


	UPROPERTY()
	FGameplayTag CustomValue1Tag;

	UPROPERTY()
	FGameplayTag CustomValue2Tag;

	UPROPERTY()
	FRockItemAssets Assets;

	/** FRockItemFragment entries, `{ "Type": "Sound", ... }`; at most one per type. */
	UPROPERTY(meta = (BaseStruct = "/Script/RockInventoryRuntime.RockItemFragment"))
	TArray<FInstancedStruct> Fragments;
};
