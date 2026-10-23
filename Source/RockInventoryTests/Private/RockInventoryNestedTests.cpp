// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "Item/Fragment/RockItemFragment_NestedInventory.h"
#include "Item/RockItemInstance.h"
#include "Item/State/RockItemState_NestedInventory.h"
#include "Library/RockInventoryLibrary.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// A nested inventory (backpack contents) follows its item between owners.
TEST_CLASS(RockInventoryNestedTests, "BRS.RockInventory.Nested")
{
	FRockInventoryFixture Fixture;
	FRockInventoryFixture Other;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	/** An item definition whose runtime instance owns a 2x2 nested inventory. */
	URockItemDefinition* MakeBackpack()
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2)};
		KeepAlive.Emplace(Config);

		URockItemDefinition* Backpack = Fixture.MakeDefinition("Backpack");
		Backpack->RuntimeInstanceClass = URockItemInstance::StaticClass();
		FRockItemFragment_NestedInventory Fragment;
		Fragment.InventoryConfig = Config;
		Backpack->Fragments.Add(FInstancedStruct::Make(Fragment));
		return Backpack;
	}

	/** Like MakeBackpack, with the nested loot settings of the nested inventory's config. */
	URockItemDefinition* MakeContainer(const FName Name, const bool bAcceptsNestedLoot, const int32 NestedLootPriority = 0)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2)};
		Config->bAcceptsNestedLoot = bAcceptsNestedLoot;
		Config->NestedLootPriority = NestedLootPriority;
		KeepAlive.Emplace(Config);

		URockItemDefinition* Container = Fixture.MakeDefinition(Name);
		Container->RuntimeInstanceClass = URockItemInstance::StaticClass();
		FRockItemFragment_NestedInventory Fragment;
		Fragment.InventoryConfig = Config;
		Container->Fragments.Add(FInstancedStruct::Make(Fragment));
		return Container;
	}

	/** Puts a new container item into Holder and returns the nested inventory it owns. */
	URockInventory* AddContainer(URockInventory* Holder, URockItemDefinition* Definition)
	{
		const FRockItemStackHandle Handle = Holder->AddItemToInventory(FRockItemStack(Definition, 1));
		return NestedOf(Holder, Handle);
	}

	TArray<const URockInventory*> FoundInventories(const URockInventory* Root, const int32 MaxDepth = URockInventoryLibrary::MaxNestedLootDepth)
	{
		TArray<FRockNestedLootTarget> Targets;
		URockInventoryLibrary::FindNestedLootInventories(Root, Targets, MaxDepth);
		TArray<const URockInventory*> Result;
		for (const FRockNestedLootTarget& Target : Targets)
		{
			Result.Add(Target.Inventory);
		}
		return Result;
	}

	URockInventory* NestedOf(const URockInventory* Inventory, const FRockItemStackHandle& Handle)
	{
		URockItemInstance* Instance = Inventory->GetItemByHandle(Handle).GetRuntimeInstance();
		return Instance ? Instance->GetNestedInventory() : nullptr;
	}

	TEST_METHOD(NewBackpack_NestedInventoryIsOwnedByTheHoldingInventory)
	{
		Fixture.InitGrid(2, 2);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(MakeBackpack(), 1, Fixture.SlotAt(0, 0));

		URockInventory* Nested = NestedOf(Fixture.Inventory, Handle);

		ASSERT_THAT(IsNotNull(Nested));
		ASSERT_THAT(IsTrue(Nested->GetOwner() == Fixture.Inventory));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::GetTopLevelOwner(Nested) == Fixture.Owner));
		ASSERT_THAT(IsTrue(Fixture.Owner->IsReplicatedSubObjectRegistered(Nested)));
	}

	TEST_METHOD(Move_AcrossInventories_NestedInventoryFollowsToTheNewOwner)
	{
		Fixture.InitGrid(2, 2);
		Other.InitGrid(2, 2);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(MakeBackpack(), 1, Fixture.SlotAt(0, 0));
		URockInventory* Nested = NestedOf(Fixture.Inventory, Handle);
		ASSERT_THAT(IsNotNull(Nested));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(1, 1))));

		const FRockItemStack Arrived = Other.Inventory->GetItemBySlotHandle(Other.SlotAt(1, 1));
		ASSERT_THAT(IsTrue(Arrived.GetRuntimeInstance() != nullptr));
		ASSERT_THAT(IsTrue(Arrived.GetRuntimeInstance()->GetNestedInventory() == Nested));
		ASSERT_THAT(IsTrue(Nested->GetOwner() == Other.Inventory));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::GetTopLevelOwner(Nested) == Other.Owner));
		ASSERT_THAT(IsTrue(Other.Owner->IsReplicatedSubObjectRegistered(Nested)));
		ASSERT_THAT(IsFalse(Fixture.Owner->IsReplicatedSubObjectRegistered(Nested)));
	}

	TEST_METHOD(Move_AcrossInventories_ItemsInsideTheBackpackFollowToo)
	{
		Fixture.InitGrid(2, 2);
		Other.InitGrid(2, 2);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(MakeBackpack(), 1, Fixture.SlotAt(0, 0));
		URockInventory* Nested = NestedOf(Fixture.Inventory, Handle);
		ASSERT_THAT(IsNotNull(Nested));
		URockItemDefinition* Apple = Fixture.MakeDefinition("Apple", 10);
		Nested->AddItemToInventory(FRockItemStack(Apple, 3));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Fixture.Inventory, Fixture.SlotAt(0, 0), Other.Inventory, Other.SlotAt(0, 0))));

		ASSERT_THAT(AreEqual(1, Nested->GetNumItemStacks()));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::GetTopLevelOwner(Nested) == Other.Owner));
	}

	TEST_METHOD(SetOwningInventory_ToNull_NestedInventoryLosesItsOwner)
	{
		Fixture.InitGrid(2, 2);
		Other.InitGrid(1, 1);
		const FRockItemStackHandle Handle = Fixture.PlaceAt(MakeBackpack(), 1, Fixture.SlotAt(0, 0));
		URockInventory* Nested = NestedOf(Fixture.Inventory, Handle);
		ASSERT_THAT(IsNotNull(Nested));
		URockItemInstance* Instance = Fixture.Inventory->GetItemByHandle(Handle).GetRuntimeInstance();

		// What FRockItemStack::TransferOwnership does when the item goes into a world item (outer: the world item actor).
		Instance->Rename(nullptr, Other.Owner);
		Instance->SetOwningInventory(nullptr);

		ASSERT_THAT(IsNull(Nested->GetOwner()));
		ASSERT_THAT(IsTrue(URockInventoryLibrary::GetTopLevelOwner(Nested) == Other.Owner));
		ASSERT_THAT(IsFalse(Fixture.Owner->IsReplicatedSubObjectRegistered(Nested)));
		ASSERT_THAT(IsTrue(Other.Owner->IsReplicatedSubObjectRegistered(Nested)));
	}

	// ---- Nested loot discovery (T-54) ----

	TEST_METHOD(NestedLoot_ConfigSettingsReachTheInventory)
	{
		Fixture.InitGrid(2, 2);
		URockInventory* Pouch = AddContainer(Fixture.Inventory, MakeContainer("Pouch", true, 7));
		URockInventory* Plain = AddContainer(Fixture.Inventory, MakeContainer("Plain", false));

		ASSERT_THAT(IsNotNull(Pouch));
		ASSERT_THAT(IsNotNull(Plain));
		ASSERT_THAT(IsTrue(Pouch->bAcceptsNestedLoot));
		ASSERT_THAT(AreEqual(7, Pouch->NestedLootPriority));
		ASSERT_THAT(IsFalse(Plain->bAcceptsNestedLoot));
	}

	TEST_METHOD(FindNestedLoot_OptedInPouch_IsFound)
	{
		Fixture.InitGrid(2, 2);
		URockInventory* Pouch = AddContainer(Fixture.Inventory, MakeContainer("Pouch", true));
		ASSERT_THAT(IsNotNull(Pouch));

		const TArray<const URockInventory*> Found = FoundInventories(Fixture.Inventory);

		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(IsTrue(Found[0] == Pouch));
	}

	TEST_METHOD(FindNestedLoot_NothingNested_IsEmpty)
	{
		Fixture.InitGrid(2, 2);
		Fixture.PlaceAt(Fixture.MakeDefinition("Apple", 10), 1, Fixture.SlotAt(0, 0));

		ASSERT_THAT(AreEqual(0, FoundInventories(Fixture.Inventory).Num()));
		ASSERT_THAT(AreEqual(0, FoundInventories(nullptr).Num()));
	}

	TEST_METHOD(FindNestedLoot_WeaponModInventory_IsNeverFound)
	{
		Fixture.InitGrid(2, 2);
		URockInventory* Mods = AddContainer(Fixture.Inventory, MakeContainer("Rifle", false));
		ASSERT_THAT(IsNotNull(Mods));
		// Something that did opt in, but sits inside a container that did not: loot must not reach through the rifle.
		URockInventory* Magazine = AddContainer(Mods, MakeContainer("Magazine", true));
		ASSERT_THAT(IsNotNull(Magazine));

		ASSERT_THAT(AreEqual(0, FoundInventories(Fixture.Inventory).Num()));
	}

	TEST_METHOD(FindNestedLoot_DepthCap_StopsAtThree)
	{
		Fixture.InitGrid(2, 2);
		URockInventory* One = AddContainer(Fixture.Inventory, MakeContainer("One", true));
		ASSERT_THAT(IsNotNull(One));
		URockInventory* Two = AddContainer(One, MakeContainer("Two", true));
		ASSERT_THAT(IsNotNull(Two));
		URockInventory* Three = AddContainer(Two, MakeContainer("Three", true));
		ASSERT_THAT(IsNotNull(Three));
		URockInventory* Four = AddContainer(Three, MakeContainer("Four", true));
		ASSERT_THAT(IsNotNull(Four));

		const TArray<const URockInventory*> Found = FoundInventories(Fixture.Inventory);
		ASSERT_THAT(AreEqual(3, Found.Num()));
		ASSERT_THAT(IsFalse(Found.Contains(Four)));
		ASSERT_THAT(IsTrue(Found.Contains(Three)));

		const TArray<const URockInventory*> Shallow = FoundInventories(Fixture.Inventory, 2);
		ASSERT_THAT(AreEqual(2, Shallow.Num()));
		ASSERT_THAT(IsFalse(Shallow.Contains(Three)));
	}

	TEST_METHOD(FindNestedLoot_TarkovStyleRoot_FindsTheBackpackAndItsPouch)
	{
		// The player inventory has only a backpack slot; the loot target is the contents of the item in it.
		Fixture.InitGrid(1, 1);
		URockInventory* BackpackContents = AddContainer(Fixture.Inventory, MakeContainer("Backpack", true, 5));
		ASSERT_THAT(IsNotNull(BackpackContents));
		URockInventory* Pouch = AddContainer(BackpackContents, MakeContainer("Pouch", true, 1));
		ASSERT_THAT(IsNotNull(Pouch));

		TArray<FRockNestedLootTarget> Targets;
		URockInventoryLibrary::FindNestedLootInventories(Fixture.Inventory, Targets);

		ASSERT_THAT(AreEqual(2, Targets.Num()));
		// Priority first: the pouch (1) comes before the backpack (5) although it is deeper.
		ASSERT_THAT(IsTrue(Targets[0].Inventory == Pouch));
		ASSERT_THAT(AreEqual(2, Targets[0].Depth));
		ASSERT_THAT(IsTrue(Targets[1].Inventory == BackpackContents));
		ASSERT_THAT(AreEqual(1, Targets[1].Depth));
	}

	TEST_METHOD(FindNestedLoot_EqualPriority_ShallowerFirstThenSlotOrder)
	{
		Fixture.InitGrid(3, 1);
		URockInventory* First = AddContainer(Fixture.Inventory, MakeContainer("First", true));
		URockInventory* Second = AddContainer(Fixture.Inventory, MakeContainer("Second", true));
		ASSERT_THAT(IsNotNull(First));
		ASSERT_THAT(IsNotNull(Second));
		URockInventory* Deep = AddContainer(First, MakeContainer("Deep", true));
		ASSERT_THAT(IsNotNull(Deep));

		const TArray<const URockInventory*> Found = FoundInventories(Fixture.Inventory);

		ASSERT_THAT(AreEqual(3, Found.Num()));
		ASSERT_THAT(IsTrue(Found[0] == First));
		ASSERT_THAT(IsTrue(Found[1] == Second));
		ASSERT_THAT(IsTrue(Found[2] == Deep));
	}

	TEST_METHOD(FindNestedLoot_ACycle_ListsEachInventoryOnce)
	{
		Fixture.InitGrid(2, 2);
		URockInventory* Outer = AddContainer(Fixture.Inventory, MakeContainer("Outer", true));
		ASSERT_THAT(IsNotNull(Outer));
		const FRockItemStackHandle InnerHandle = Outer->AddItemToInventory(FRockItemStack(MakeContainer("Inner", true), 1));
		URockItemInstance* InnerInstance = Outer->GetItemByHandle(InnerHandle).GetRuntimeInstance();
		ASSERT_THAT(IsNotNull(InnerInstance));
		// Point the inner item's nested inventory back at the outer one: Outer contains an item whose contents are Outer.
		InnerInstance->FindOrAddState<FRockItemState_NestedInventory>().NestedInventory = Outer;

		const TArray<const URockInventory*> Found = FoundInventories(Fixture.Inventory);

		ASSERT_THAT(AreEqual(1, Found.Num()));
		ASSERT_THAT(IsTrue(Found[0] == Outer));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
