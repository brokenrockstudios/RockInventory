// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "Item/Fragment/RockItemFragment_NestedInventory.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "RockInventoryTestListener.h"
#include "RockInventoryTestTags.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Loot placement across the root and its opted-in nested inventories (T-55): the plan holds (inventory, section) entries, both passes span all of them.
// "Herb" is a Food item in these tests, a Weapon is anything that is not a herb.
TEST_CLASS(RockInventoryNestedLootTests, "BRS.RockInventory.Loot.Nested")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	static FGameplayTagContainer Tags(const FGameplayTag& Tag) { return FGameplayTagContainer(Tag); }
	static int32 AsInt(ERockLootIntent Intent) { return static_cast<int32>(Intent); }

	URockItemDefinition* Herb(FName Id = "Herb", int32 MaxStack = 10) { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Food), MaxStack); }
	URockItemDefinition* Sword(FName Id = "Sword") { return Fixture.MakeTaggedDefinition(Id, Tags(RockInventoryTestTags::Weapon)); }

	/** A section for herbs only. */
	static FRockInventorySectionInfo HerbSection(const FGameplayTag& Tag, int32 Columns, int32 Rows)
	{
		return FRockInventoryFixture::MakeSection(Tag, Columns, Rows, Tags(RockInventoryTestTags::Food));
	}

	/** Puts a container item into Holder whose nested inventory has the given sections and nested loot settings; returns that nested inventory. */
	URockInventory* AddContainer(
		URockInventory* Holder, FName Name, const TArray<FRockInventorySectionInfo>& Sections, bool bAcceptsNestedLoot, int32 NestedLootPriority = 0)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = Sections;
		Config->bAcceptsNestedLoot = bAcceptsNestedLoot;
		Config->NestedLootPriority = NestedLootPriority;
		KeepAlive.Emplace(Config);

		URockItemDefinition* Container = Fixture.MakeDefinition(Name);
		Container->RuntimeInstanceClass = URockItemInstance::StaticClass();
		FRockItemFragment_NestedInventory Fragment;
		Fragment.InventoryConfig = Config;
		Container->Fragments.Add(FInstancedStruct::Make(Fragment));

		const FRockItemStackHandle Handle = Holder->AddItemToInventory(FRockItemStack(Container, 1));
		URockItemInstance* Instance = Holder->GetItemByHandle(Handle).GetRuntimeInstance();
		return Instance ? Instance->GetNestedInventory() : nullptr;
	}

	/** Previews, loots, checks the two agree on every placement (inventory included), and returns the real result. */
	FRockLootResult LootWith(URockItemDefinition* Definition, int32 Count = 1, ERockLootIntent Intent = ERockLootIntent::Store)
	{
		FRockLootParams Params;
		Params.Intent = AsInt(Intent);
		const FRockItemStack Stack(Definition, Count);
		const FRockLootResult Preview = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, Stack, Params);
		FRockLootResult Real;
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, Stack, Params, Real);
		bool bAgree = Preview.Placements.Num() == Real.Placements.Num() && Preview.Excess == Real.Excess;
		for (int32 Index = 0; bAgree && Index < Real.Placements.Num(); ++Index)
		{
			bAgree = Preview.Placements[Index].Slot.GetInventory() == Real.Placements[Index].Slot.GetInventory()
				&& Preview.Placements[Index].Slot.GetSlotHandle() == Real.Placements[Index].Slot.GetSlotHandle()
				&& Preview.Placements[Index].Count == Real.Placements[Index].Count;
		}
		if (!bAgree)
		{
			AddError(TEXT("PreviewLoot and LootItemToInventory disagree"));
		}
		return Real;
	}

	/** The inventory of the first placement, or null when nothing was placed. */
	static const URockInventory* FirstInventory(const FRockLootResult& Result)
	{
		return Result.Placements.IsEmpty() ? nullptr : Result.Placements[0].Slot.GetInventory();
	}

	void InitRoot(int32 Columns = 3, int32 Rows = 1)
	{
		Fixture.InitGrid(Columns, Rows);
	}

	TEST_METHOD(Herb_GoesToTheOptedInPouchBeforeTheBackpack)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herb());

		ASSERT_THAT(IsTrue(FirstInventory(Result) == Pouch));
		ASSERT_THAT(AreEqual(1, Pouch->GetNumItemStacks()));
		// The pouch itself is the only stack in the root
		ASSERT_THAT(AreEqual(1, Fixture.Inventory->GetNumItemStacks()));
	}

	TEST_METHOD(Herb_AtEqualPriority_RootSectionsComeBeforeNested)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, 0);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herb());

		ASSERT_THAT(IsTrue(FirstInventory(Result) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Herb_PouchWithAPreference_BeatsTheRootRegardlessOfPriority)
	{
		InitRoot();
		FRockInventorySectionInfo Preferred = FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1);
		Preferred.SetLootPreference(FGameplayTagQuery::MakeQuery_MatchAnyTags(Tags(RockInventoryTestTags::Food)));
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {Preferred}, true, 50);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herb());

		ASSERT_THAT(IsTrue(FirstInventory(Result) == Pouch));
	}

	TEST_METHOD(NestedPriority_OrdersPouchesAgainstEachOther)
	{
		InitRoot(3, 1);
		URockInventory* Late = AddContainer(Fixture.Inventory, "Late", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1)}, true, 5);
		URockInventory* Early = AddContainer(Fixture.Inventory, "Early", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1)}, true, -5);
		ASSERT_THAT(IsNotNull(Late));
		ASSERT_THAT(IsNotNull(Early));

		const FRockLootResult First = LootWith(Herb("H1", 1));
		const FRockLootResult Second = LootWith(Herb("H2", 1));

		ASSERT_THAT(IsTrue(FirstInventory(First) == Early));
		// Early is full: the second herb goes to Late, not the root (the root comes after a negative priority but before a positive one)
		ASSERT_THAT(IsTrue(FirstInventory(Second) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Late->GetNumItemStacks()));
	}

	TEST_METHOD(Weapon_SkipsThePouchByItsFilter)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Sword());

		ASSERT_THAT(IsTrue(FirstInventory(Result) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::DescribeLootPlan(Fixture.Inventory, FRockItemStack(Sword("S2"), 1), FRockLootParams()).Contains(TEXT("skipped nested"))));
	}

	TEST_METHOD(Herb_WhenThePouchIsFull_FallsBackToTheRoot)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult First = LootWith(Herb("H1", 1));
		const FRockLootResult Second = LootWith(Herb("H2", 1));

		ASSERT_THAT(IsTrue(FirstInventory(First) == Pouch));
		ASSERT_THAT(IsTrue(FirstInventory(Second) == Fixture.Inventory));
		ASSERT_THAT(IsTrue(Second.IsFullyPlaced()));
	}

	TEST_METHOD(PouchWithoutOptIn_NeverReceivesLoot)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, false);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herb());

		ASSERT_THAT(IsTrue(FirstInventory(Result) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Store_HonorsTheIntentMaskOfNestedSections)
	{
		InitRoot();
		FRockInventorySectionInfo EquipOnly = HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1);
		EquipOnly.SetAcceptedLootIntents(ERockLootIntent::Equip);
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {EquipOnly}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult StoreCall = LootWith(Herb("H1"), 1, ERockLootIntent::Store);
		ASSERT_THAT(IsTrue(FirstInventory(StoreCall) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));

		// The same section is used by an Equip call (the root's Store-only sections are not)
		const FRockLootResult EquipCall = LootWith(Herb("H2"), 1, ERockLootIntent::Equip);
		ASSERT_THAT(IsTrue(FirstInventory(EquipCall) == Pouch));
		ASSERT_THAT(AreEqual(1, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Merge_TopsUpThePartialStackInThePouchFirst)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));
		URockItemDefinition* Herbs = Herb();
		FRockLootResult Setup;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Herbs, 3), FRockLootParams(), Setup)));
		ASSERT_THAT(IsTrue(FirstInventory(Setup) == Pouch));

		const FRockLootResult Result = LootWith(Herbs, 4);

		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(IsFalse(Result.Placements[0].bNewStack));
		ASSERT_THAT(IsTrue(FirstInventory(Result) == Pouch));
		ASSERT_THAT(AreEqual(1, Pouch->GetNumItemStacks()));
		ASSERT_THAT(AreEqual(7, Pouch->GetItemBySlotHandle(Result.Placements[0].Slot.GetSlotHandle()).GetStackCount()));
	}

	TEST_METHOD(Merge_RunsBeforeNewStacks_AcrossInventories)
	{
		InitRoot();
		URockItemDefinition* Herbs = Herb();
		// A partial stack in the root first, then the pouch (which is tried first) with only empty space
		FRockLootResult Setup;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Herbs, 5), FRockLootParams(), Setup)));
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herbs, 3);

		// Pass 1 spans every target: the root's partial stack is topped up, the empty pouch is not used
		ASSERT_THAT(AreEqual(1, Result.Placements.Num()));
		ASSERT_THAT(IsFalse(Result.Placements[0].bNewStack));
		ASSERT_THAT(IsTrue(FirstInventory(Result) == Fixture.Inventory));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Excess_SpillsFromThePouchIntoTheRoot)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 1, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));

		const FRockLootResult Result = LootWith(Herb(), 14);

		ASSERT_THAT(IsTrue(Result.IsFullyPlaced()));
		ASSERT_THAT(AreEqual(2, Result.Placements.Num()));
		ASSERT_THAT(IsTrue(Result.Placements[0].Slot.GetInventory() == Pouch));
		ASSERT_THAT(AreEqual(10, Result.Placements[0].Count));
		ASSERT_THAT(IsTrue(Result.Placements[1].Slot.GetInventory() == Fixture.Inventory));
		ASSERT_THAT(AreEqual(4, Result.Placements[1].Count));
	}

	TEST_METHOD(BackpackSlotRoot_PlacesIntoTheBackpacksContents)
	{
		// Tarkov style: the root has only an equipment slot for the backpack item; everything else lives in the backpack's inventory
		Fixture.Init({FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionPrimary, 1, 1, Tags(RockInventoryTestTags::Wieldable))});
		URockInventory* Contents = AddContainer(Fixture.Inventory, "Backpack", {FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 2, 2)}, true);
		ASSERT_THAT(IsNotNull(Contents));

		const FRockLootResult Result = LootWith(Herb("H1", 1), 3);

		ASSERT_THAT(IsTrue(Result.IsFullyPlaced()));
		ASSERT_THAT(AreEqual(3, Result.Placements.Num()));
		for (const FRockLootPlacement& Placement : Result.Placements)
		{
			ASSERT_THAT(IsTrue(Placement.Slot.GetInventory() == Contents));
		}
		ASSERT_THAT(AreEqual(3, Contents->GetNumItemStacks()));
	}

	TEST_METHOD(BatchPreview_FillsThePouchThenTheRoot_AsSeparateCallsWould)
	{
		InitRoot(2, 1);
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));
		TArray<FRockItemStack> Stacks;
		for (const FName Id : {FName("H1"), FName("H2"), FName("H3")})
		{
			Stacks.Add(FRockItemStack(Herb(Id, 1), 1));
		}

		const TArray<FRockLootResult> Batch = URockInventoryLibrary::PreviewLoot(Fixture.Inventory, Stacks, FRockLootParams());

		ASSERT_THAT(AreEqual(3, Batch.Num()));
		ASSERT_THAT(IsTrue(FirstInventory(Batch[0]) == Pouch));
		ASSERT_THAT(IsTrue(FirstInventory(Batch[1]) == Pouch));
		ASSERT_THAT(IsTrue(FirstInventory(Batch[2]) == Fixture.Inventory));
		ASSERT_THAT(AreNotEqual(Batch[0].Placements[0].Slot.GetSlotHandle(), Batch[1].Placements[0].Slot.GetSlotHandle()));
		// A preview changes nothing
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Loot_IntoNested_RaisesEventsOnTheNestedInventoryOnly)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));
		URockInventoryTestListener* PouchListener = NewObject<URockInventoryTestListener>(GetTransientPackage());
		URockInventoryTestListener* RootListener = NewObject<URockInventoryTestListener>(GetTransientPackage());
		KeepAlive.Emplace(PouchListener);
		KeepAlive.Emplace(RootListener);
		Pouch->OnSlotChanged.AddDynamic(PouchListener, &URockInventoryTestListener::OnSlotChanged);
		Pouch->OnItemChanged.AddDynamic(PouchListener, &URockInventoryTestListener::OnItemChanged);
		Fixture.Inventory->OnSlotChanged.AddDynamic(RootListener, &URockInventoryTestListener::OnSlotChanged);
		Fixture.Inventory->OnItemChanged.AddDynamic(RootListener, &URockInventoryTestListener::OnItemChanged);

		LootWith(Herb());

		ASSERT_THAT(IsTrue(PouchListener->SlotDeltas.Num() >= 1));
		ASSERT_THAT(IsTrue(PouchListener->ItemDeltas.Num() >= 1));
		ASSERT_THAT(AreEqual(0, RootListener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(0, RootListener->ItemDeltas.Num()));
	}

	TEST_METHOD(Loot_OnAClient_IsRefusedAndTheNestedInventoryStaysEmpty)
	{
		InitRoot();
		URockInventory* Pouch = AddContainer(Fixture.Inventory, "Pouch", {HerbSection(RockInventoryTags::Inventory_Section_Pockets, 2, 1)}, true, -1);
		ASSERT_THAT(IsNotNull(Pouch));
		// The nested inventory resolves to the same owning actor, so the authority check on the root covers it
		ASSERT_THAT(IsTrue(Pouch->GetOwningActor() == Fixture.Owner));
		Fixture.Owner->SetRole(ROLE_SimulatedProxy);
		TestRunner->AddExpectedMessagePlain(TEXT("Inventory change refused"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		FRockLootResult Result;

		const bool bPlaced = URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Herb(), 1), FRockLootParams(), Result);

		ASSERT_THAT(IsFalse(bPlaced));
		ASSERT_THAT(AreEqual(0, Pouch->GetNumItemStacks()));
	}

	TEST_METHOD(Swap_NeverDisplacesAStackInANestedInventory)
	{
		// Equip-only with swap: the swap pass looks at the root's occupied slots only
		Fixture.Init({FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionPrimary, 1, 1, Tags(RockInventoryTestTags::Weapon)).SetAcceptedLootIntents(ERockLootIntent::Equip),
			FRockInventoryFixture::MakeSection(RockInventoryTags::Inventory_Section_Backpack, 1, 1)});
		FRockInventorySectionInfo NestedEquip = FRockInventoryFixture::MakeSection(RockInventoryTestTags::SectionSecondary, 1, 1, Tags(RockInventoryTestTags::Weapon));
		NestedEquip.SetAcceptedLootIntents(ERockLootIntent::Equip);
		URockInventory* Holster = AddContainer(Fixture.Inventory, "Holster", {NestedEquip}, true, -1);
		ASSERT_THAT(IsNotNull(Holster));
		FRockLootParams Equip;
		Equip.Intent = AsInt(ERockLootIntent::Equip);
		Equip.bAllowSwap = true;
		FRockLootResult Setup;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Sword("S1"), 1), Equip, Setup)));
		ASSERT_THAT(IsTrue(FirstInventory(Setup) == Holster));
		FRockLootResult Second;
		ASSERT_THAT(IsTrue(URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Sword("S2"), 1), Equip, Second)));
		ASSERT_THAT(IsTrue(FirstInventory(Second) == Fixture.Inventory));

		// Both equipment slots are full now; a third sword may swap, but only the root's occupant can be displaced
		FRockLootResult Third;
		URockInventoryLibrary::LootItemToInventory(Fixture.Inventory, FRockItemStack(Sword("S3"), 1), Equip, Third);

		ASSERT_THAT(IsTrue(Third.bSwapped ? Third.DisplacedSlot.GetInventory() == Fixture.Inventory : Third.Placements.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Holster->GetNumItemStacks()));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
