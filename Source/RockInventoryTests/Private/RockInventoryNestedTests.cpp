// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryConfig.h"
#include "Item/Fragment/RockItemFragment_NestedInventory.h"
#include "Item/RockItemInstance.h"
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
};

#endif // WITH_DEV_AUTOMATION_TESTS
