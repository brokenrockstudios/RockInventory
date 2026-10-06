// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

void FRockInventoryFixture::Init(const TArray<FRockInventorySectionInfo>& Sections)
{
	Owner = &Spawner.SpawnActor<AActor>();

	URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
	Config->InventoryTabs = Sections;
	KeepAlive.Emplace(Config);

	Inventory = NewObject<URockInventory>(Owner);
	Inventory->Owner = Owner;
	KeepAlive.Emplace(Inventory);

	Inventory->Init(Config);
}

void FRockInventoryFixture::InitGrid(int32 Columns, int32 Rows, ERockItemSizePolicy SizePolicy)
{
	Init({FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, Columns, Rows, SizePolicy)});
}

URockItemDefinition* FRockInventoryFixture::MakeDefinition(FName ItemId, int32 MaxStack, FIntPoint GridSize)
{
	URockItemDefinition* Definition = NewObject<URockItemDefinition>(GetTransientPackage());
	Definition->ItemId = ItemId;
	Definition->MaxStackCount = MaxStack;
	Definition->GridSize = GridSize;
	KeepAlive.Emplace(Definition);
	return Definition;
}

URockItemDefinition* FRockInventoryFixture::MakeTaggedDefinition(
	FName ItemId, const FGameplayTagContainer& ItemTags, int32 MaxStack, FIntPoint GridSize)
{
	URockItemDefinition* Definition = MakeDefinition(ItemId, MaxStack, GridSize);
	Definition->ItemTags = ItemTags;
	Definition->RebuildCachedTags();
	return Definition;
}

FRockInventorySectionInfo FRockInventoryFixture::MakeSection(
	const FGameplayTag& SectionTag, int32 Columns, int32 Rows,
	const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& MetaTags)
{
	FRockInventorySectionInfo Section(SectionTag, 0, Columns, Rows);
	if (!RequiredTags.IsEmpty())
	{
		Section.SetSectionFilter(FGameplayTagQuery::MakeQuery_MatchAnyTags(RequiredTags));
	}
	Section.SetMetaTags(MetaTags);
	return Section;
}

FRockInventorySlotHandle FRockInventoryFixture::SlotAt(const FGameplayTag& Section, int32 Column, int32 Row) const
{
	const FRockInventorySectionInfo& Info = Inventory->GetSectionInfo(Section);
	return FRockInventorySlotHandle(Info.GetFirstSlotIndex() + Row * Info.GetColumns() + Column);
}

FRockInventorySlotHandle FRockInventoryFixture::SlotAt(int32 Column, int32 Row) const
{
	return SlotAt(RockInventoryTags::Inventory_Section_Backpack, Column, Row);
}

FRockItemStackHandle FRockInventoryFixture::PlaceAt(URockItemDefinition* Definition, int32 Count, FRockInventorySlotHandle Slot)
{
	const FRockItemStackHandle Handle = Inventory->AddItemToInventory(FRockItemStack(Definition, Count));
	FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(Slot);
	Entry.ItemHandle = Handle;
	Inventory->SetSlotByHandle(Slot, Entry);
	return Handle;
}

bool FRockInventoryFixture::Loot(URockItemDefinition* Definition, int32 Count, FRockInventorySlotHandle& OutSlot, int32& OutExcess)
{
	FRockLootResult Result;
	const bool bPlaced = URockInventoryLibrary::LootItemToInventory(Inventory, FRockItemStack(Definition, Count), FRockLootParams(), Result);
	OutExcess = Result.Excess;
	// OutSlot is the new stack's slot, and stays untouched when everything merged into existing stacks
	if (const FRockLootPlacement* NewStack = Result.FindNewStack())
	{
		OutSlot = NewStack->Slot.GetSlotHandle();
	}
	return bPlaced;
}
