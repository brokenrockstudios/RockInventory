// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockItemRecordProvider.h"

#include "Item/RockItemDefinition.h"
#include "RockDataRegistry.h"
#include "RockInventoryDataLogging.h"
#include "RockItemRecord.h"

DEFINE_LOG_CATEGORY(LogRockInventoryData);

namespace RockItemRecordProviderPrivate
{
	const FName NameField(TEXT("Name"));
	const FName DisplayNameField(TEXT("DisplayName"));
	const FName DescriptionField(TEXT("Description"));

	FText FieldText(const FRockDataRegistry& Source, FName ItemId, FName Field)
	{
		return Source.FindText(Source.GetTextKey(RockItemData::ItemTypeName, ItemId, Field));
	}

	/** Rewrites Definition from Record. Warns once when stacks that are already placed may no longer fit. */
	void Apply(const FRockDataRegistry& Source, URockItemDefinition& Definition, FName ItemId, const FRockItemRecord& Record, bool bExisting)
	{
		if (bExisting)
		{
			const bool bStackShrank = Record.MaxStackCount < Definition.MaxStackCount;
			const bool bGridGrew = Record.GridSize.X > Definition.GridSize.X || Record.GridSize.Y > Definition.GridSize.Y;
			if (bStackShrank || bGridGrew)
			{
				FString What;
				if (bStackShrank)
				{
					What = FString::Printf(TEXT("MaxStackCount %d -> %d"), Definition.MaxStackCount, Record.MaxStackCount);
				}
				if (bGridGrew)
				{
					What += FString::Printf(TEXT("%sGridSize %dx%d -> %dx%d"), bStackShrank ? TEXT(", ") : TEXT(""),
						Definition.GridSize.X, Definition.GridSize.Y, Record.GridSize.X, Record.GridSize.Y);
				}
				UE_LOG(LogRockInventoryData, Warning, TEXT("Item %s reloaded with %s: stacks already placed keep their count and footprint"), *ItemId.ToString(), *What);
			}
		}

		Definition.ItemId = ItemId;
		Definition.Name = FieldText(Source, ItemId, NameField);
		Definition.DisplayName = FieldText(Source, ItemId, DisplayNameField);
		Definition.Description = FieldText(Source, ItemId, DescriptionField);
		Definition.IconData.Description = Definition.Description;
		Definition.MaxStackCount = Record.MaxStackCount;
		Definition.GridSize = Record.GridSize;
		Definition.ItemType = FGameplayTagContainer::CreateFromArray(Record.ItemType);
		Definition.ItemTags = FGameplayTagContainer::CreateFromArray(Record.ItemTags);
		Definition.ItemRarity = Record.ItemRarity;
		Definition.Weight = Record.Weight;
		Definition.ItemValue = Record.ItemValue;
		Definition.CustomValue1Tag = Record.CustomValue1Tag;
		Definition.CustomValue2Tag = Record.CustomValue2Tag;
		Definition.SetAssets(Record.Assets.Icon.Path, Record.Assets.Mesh.Path, Record.Assets.SkeletalMesh.Path, Record.Assets.SpawnedActorClass.Path, Record.Assets.RuntimeInstanceClass.Path);


		Definition.Fragments = Record.Fragments;
		for (FInstancedStruct& Fragment : Definition.Fragments)
		{
			if (FRockItemFragment* FragmentPtr = Fragment.GetMutablePtr<FRockItemFragment>())
			{
				FragmentPtr->OnPostLoad(&Definition);
			}
		}
		Definition.SortFragments();
		Definition.RebuildCachedTags();
	}
}

FRockItemRecordProvider::FRockItemRecordProvider(const FRockDataRegistry& InSource, UObject* InOuter)
	: Source(InSource)
	, Outer(InOuter)
{
}

bool FRockItemRecordProvider::Sync()
{
	UObject* OuterObject = Outer.Get();
	if (!OuterObject)
	{
		return false;
	}

	bool bSetChanged = false;
	TSet<FName> Current;
	for (const FName ItemId : Source.GetIDs(RockItemData::ItemTypeName))
	{
		const FRockItemRecord* Record = Source.FindDefinition<FRockItemRecord>(RockItemData::ItemTypeName, ItemId);
		if (!Record)
		{
			continue;
		}
		Current.Add(ItemId);
		TObjectPtr<URockItemDefinition>& Slot = Definitions.FindOrAdd(ItemId);
		const bool bExisting = Slot != nullptr;
		if (!Slot)
		{
			const FName ObjectName(*(TEXT("Item_") + ItemId.ToString()));
			// A dropped record that came back before GC left its object behind: adopt it, so the pointer stays.
			Slot = FindObject<URockItemDefinition>(OuterObject, *ObjectName.ToString());
			if (!Slot)
			{
				Slot = NewObject<URockItemDefinition>(OuterObject, ObjectName, RF_Transient);
			}
			bSetChanged = true;
		}
		RockItemRecordProviderPrivate::Apply(Source, *Slot, ItemId, *Record, bExisting);
	}

	for (auto It = Definitions.CreateIterator(); It; ++It)
	{
		if (!Current.Contains(It.Key()))
		{
			It.RemoveCurrent();
			bSetChanged = true;
		}
	}
	return bSetChanged;
}

URockItemDefinition* FRockItemRecordProvider::FindDefinition(FName ItemId) const
{
	const TObjectPtr<URockItemDefinition>* Found = Definitions.Find(ItemId);
	return Found ? Found->Get() : nullptr;
}

void FRockItemRecordProvider::EnumerateItemDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const
{
	const_cast<FRockItemRecordProvider*>(this)->Sync();
	for (const TPair<FName, TObjectPtr<URockItemDefinition>>& Pair : Definitions)
	{
		OutDefinitions.Add(Pair.Value);
	}
}

FString FRockItemRecordProvider::DescribeItemSource(const URockItemDefinition* Definition) const
{
	if (Definition)
	{
		if (const FRockDataRecord* Record = Source.FindRecord(RockItemData::ItemTypeName, Definition->ItemId))
		{
			return Record->SourceFile;
		}
	}
	return IRockItemDefinitionProvider::DescribeItemSource(Definition);
}

void FRockItemRecordProvider::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(Definitions);
}
