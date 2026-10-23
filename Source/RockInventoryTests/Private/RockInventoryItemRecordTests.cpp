// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Item/RockItemDefinition.h"
#include "RockDataRegistry.h"
#include "RockInventoryTestFragments.h"
#include "RockItemRecord.h"
#include "RockItemRecordProvider.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace RockInventoryItemRecordTestsPrivate
{
	const FName GameRoot(TEXT("Game"));
}

/** Item records materialized as URockItemDefinition objects (T-988). Records come from strings, no world. */
TEST_CLASS(RockInventoryItemRecordTests, "BRS.RockInventory.Data.Records")
{
	FRockDataRegistry Records;
	TUniquePtr<FRockItemRecordProvider> Provider;

	BEFORE_EACH()
	{
		const FRockDataTypeDesc* Desc = FRockDataRegistry::Get().FindType(RockItemData::ItemTypeName);
		ASSERT_THAT(IsTrue(Desc != nullptr));
		FString Error;
		ASSERT_THAT(IsTrue(Records.RegisterType(*Desc, Error)));
		Records.AddRoot(RockInventoryItemRecordTestsPrivate::GameRoot, 0);
		Provider = MakeUnique<FRockItemRecordProvider>(Records, URockItemRegistrySubsystem::GetInstance());
	}

	void Add(const TCHAR* Path, const TCHAR* Text) { Records.AddSource(RockInventoryItemRecordTestsPrivate::GameRoot, Path, Text); }

	URockItemDefinition* Find(const TCHAR* ID) const { return Provider->FindDefinition(FName(ID)); }

	bool HasError(const TCHAR* Substring) const
	{
		return Records.GetErrors().ContainsByPredicate([Substring](const FRockDataError& E) { return E.ToString().Contains(Substring); });
	}

	TEST_METHOD(ItemType_IsRegisteredByTheModule)
	{
		const FRockDataTypeDesc* Desc = FRockDataRegistry::Get().FindType(RockItemData::ItemTypeName);
		ASSERT_THAT(IsTrue(Desc != nullptr));
		ASSERT_THAT(AreEqual(FString(TEXT("Items")), Desc->Folder));
		ASSERT_THAT(AreEqual(3, Desc->TextFields.Num()));
	}

	TEST_METHOD(Sync_RecordFromString_FillsTheDefinition)
	{
		Add(TEXT("Items/TestRecFull.jsonc"), TEXT(R"({
			"MaxStackCount": 120,
			"GridSize": { "X": 2, "Y": 1 },
			"ItemType": ["Test.RockInventory.Item.Weapon"],
			"ItemTags": ["Test.RockInventory.Item.Wieldable"],
			"ItemRarity": "Test.RockInventory.Item.Food",
			"Weight": 4500,
			"ItemValue": 7,
			"CustomValue1Tag": "Test.RockInventory.Section.MetaB",
			"Assets": { "Icon": "/Game/UI/T_Icon", "Mesh": "/Game/Meshes/SM_Thing", "SpawnedActorClass": "/Game/Actors/BP_Thing" }
		})"));
		ASSERT_THAT(IsTrue(Records.Build()));
		ASSERT_THAT(IsTrue(Provider->Sync()));

		URockItemDefinition* Definition = Find(TEXT("TestRecFull"));
		ASSERT_THAT(IsNotNull(Definition));
		ASSERT_THAT(AreEqual(FName(TEXT("TestRecFull")), Definition->ItemId));
		ASSERT_THAT(AreEqual(FString(TEXT("Item_TestRecFull")), Definition->GetName()));
		ASSERT_THAT(AreEqual(URockItemRegistrySubsystem::GetInstance(), Definition->GetOuter()));
		ASSERT_THAT(AreEqual(120, Definition->MaxStackCount));
		ASSERT_THAT(AreEqual(FIntPoint(2, 1), Definition->GridSize));
		ASSERT_THAT(AreEqual(static_cast<int64>(4500), Definition->Weight));
		ASSERT_THAT(AreEqual(7, Definition->ItemValue));
		ASSERT_THAT(IsTrue(Definition->ItemType.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Test.RockInventory.Item.Weapon")))));
		ASSERT_THAT(IsTrue(Definition->GetAllTags().HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Test.RockInventory.Item.Wieldable")))));
		ASSERT_THAT(IsTrue(Definition->GetAllTags().HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Test.RockInventory.Item.Weapon")))));
		ASSERT_THAT(IsTrue(Definition->CustomValue1Tag == FGameplayTag::RequestGameplayTag(TEXT("Test.RockInventory.Section.MetaB"))));
		ASSERT_THAT(AreEqual(FString(TEXT("/Game/UI/T_Icon.T_Icon")), Definition->IconData.Icon.ToSoftObjectPath().ToString()));
		ASSERT_THAT(AreEqual(FString(TEXT("/Game/Meshes/SM_Thing.SM_Thing")), Definition->ItemMesh.ToSoftObjectPath().ToString()));
		ASSERT_THAT(IsTrue(Definition->ItemSkeletalMesh.IsNull()));
		ASSERT_THAT(IsFalse(Definition->ActorClass.IsNull()));
		ASSERT_THAT(IsTrue(Definition->RuntimeInstanceClass.IsNull()));
	}

	TEST_METHOD(Sync_ChildRecord_ChangesOnlyTheFieldItOverrides)
	{
		Add(TEXT("Items/TestRecBase.jsonc"), TEXT(R"({ "Abstract": true, "MaxStackCount": 10, "Weight": 200, "ItemValue": 4 })"));
		Add(TEXT("Items/TestRecChild.jsonc"), TEXT(R"({ "Extends": "TestRecBase", "MaxStackCount": 99 })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();

		ASSERT_THAT(IsNull(Find(TEXT("TestRecBase"))));
		URockItemDefinition* Child = Find(TEXT("TestRecChild"));
		ASSERT_THAT(IsNotNull(Child));
		ASSERT_THAT(AreEqual(99, Child->MaxStackCount));
		ASSERT_THAT(AreEqual(static_cast<int64>(200), Child->Weight));
		ASSERT_THAT(AreEqual(4, Child->ItemValue));
	}

	TEST_METHOD(Sync_AfterReload_KeepsTheObjectWithNewValues)
	{
		Add(TEXT("Items/TestRecReload.jsonc"), TEXT(R"({ "MaxStackCount": 10, "ItemValue": 1 })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();
		URockItemDefinition* Before = Find(TEXT("TestRecReload"));
		ASSERT_THAT(IsNotNull(Before));

		Records.Clear();
		Add(TEXT("Items/TestRecReload.jsonc"), TEXT(R"({ "MaxStackCount": 10, "ItemValue": 5 })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		ASSERT_THAT(IsFalse(Provider->Sync()));

		ASSERT_THAT(AreEqual(Before, Find(TEXT("TestRecReload"))));
		ASSERT_THAT(AreEqual(5, Before->ItemValue));
	}

	TEST_METHOD(Sync_ReloadThatShrinksStackOrGrowsGrid_WarnsOnce)
	{
		Add(TEXT("Items/TestRecWarn.jsonc"), TEXT(R"({ "MaxStackCount": 50 })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();

		Records.Clear();
		Add(TEXT("Items/TestRecWarn.jsonc"), TEXT(R"({ "MaxStackCount": 20, "GridSize": { "X": 2, "Y": 2 } })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		TestRunner->AddExpectedMessagePlain(TEXT("Item TestRecWarn reloaded with MaxStackCount 50 -> 20, GridSize 1x1 -> 2x2"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Provider->Sync();
		// Syncing again with nothing changed does not warn again.
		Provider->Sync();
	}

	TEST_METHOD(Sync_RecordRemoved_DropsTheDefinition)
	{
		Add(TEXT("Items/TestRecGone.jsonc"), TEXT("{}"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();
		ASSERT_THAT(IsNotNull(Find(TEXT("TestRecGone"))));

		Records.Clear();
		ASSERT_THAT(IsTrue(Records.Build()));
		ASSERT_THAT(IsTrue(Provider->Sync()));
		ASSERT_THAT(IsNull(Find(TEXT("TestRecGone"))));
	}

	TEST_METHOD(Sync_TextFields_ResolveFromTheTextFiles)
	{
		Add(TEXT("Items/TestRecText.jsonc"), TEXT("{}"));
		Add(TEXT("Text/en.jsonc"), TEXT(R"({ "Item.TestRecText.Name": "Rifle", "Item.TestRecText.DisplayName": "Assault Rifle", "Item.TestRecText.Description": "Loud." })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();

		URockItemDefinition* Definition = Find(TEXT("TestRecText"));
		ASSERT_THAT(IsNotNull(Definition));
		ASSERT_THAT(AreEqual(FString(TEXT("Rifle")), Definition->Name.ToString()));
		ASSERT_THAT(AreEqual(FString(TEXT("Assault Rifle")), Definition->DisplayName.ToString()));
		ASSERT_THAT(AreEqual(FString(TEXT("Loud.")), Definition->Description.ToString()));
	}

	TEST_METHOD(Build_FragmentList_FillsTheDefinitionsFragments)
	{
		Add(TEXT("Items/TestRecFrag.jsonc"), TEXT(R"({ "Fragments": [ { "Type": "RockTestFragment_CombineLimit", "Limit": 4 } ] })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		Provider->Sync();

		URockItemDefinition* Definition = Find(TEXT("TestRecFrag"));
		ASSERT_THAT(IsNotNull(Definition));
		const FRockTestFragment_CombineLimit* Fragment = Definition->FindFragment<FRockTestFragment_CombineLimit>();
		ASSERT_THAT(IsNotNull(Fragment));
		ASSERT_THAT(AreEqual(4, Fragment->Limit));
	}

	TEST_METHOD(Build_UnknownFragmentType_IsALoadError)
	{
		Add(TEXT("Items/TestRecBadFrag.jsonc"), TEXT(R"({ "Fragments": [ { "Type": "NoSuchFragment" } ] })"));
		ASSERT_THAT(IsFalse(Records.Build()));
		ASSERT_THAT(IsTrue(HasError(TEXT("Fragments[0].Type: unknown struct \"NoSuchFragment\""))));
		Provider->Sync();
		ASSERT_THAT(IsNull(Find(TEXT("TestRecBadFrag"))));
	}

	TEST_METHOD(Build_InvalidValues_AreLoadErrors)
	{
		Add(TEXT("Items/TestRecBadStack.jsonc"), TEXT(R"({ "MaxStackCount": 0 })"));
		Add(TEXT("Items/TestRecBadGrid.jsonc"), TEXT(R"({ "GridSize": { "X": 0, "Y": 1 } })"));
		ASSERT_THAT(IsFalse(Records.Build()));
		ASSERT_THAT(IsTrue(HasError(TEXT("MaxStackCount: 0 is below 1"))));
		ASSERT_THAT(IsTrue(HasError(TEXT("GridSize: 0x1 needs at least 1x1"))));
	}

	TEST_METHOD(ItemRegistry_WithProviderRegistered_FindsRecordDefinitions)
	{
		Add(TEXT("Items/TestRecRegistry.jsonc"), TEXT(R"({ "MaxStackCount": 3 })"));
		ASSERT_THAT(IsTrue(Records.Build()));
		URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance();
		Registry->RegisterProvider(Provider.Get());

		URockItemDefinition* Found = Registry->FindDefinition(TEXT("TestRecRegistry"));
		Registry->UnregisterProvider(Provider.Get());
		ASSERT_THAT(IsNotNull(Found));
		ASSERT_THAT(AreEqual(Found, Provider->FindDefinition(TEXT("TestRecRegistry"))));
		ASSERT_THAT(AreEqual(3, Found->MaxStackCount));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
