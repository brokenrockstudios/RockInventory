// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemDefinition.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Library/RockItemStackLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

namespace RockInventoryItemRegistryTestsPrivate
{
	/** A provider whose definitions and source label the test controls. */
	struct FTestProvider : IRockItemDefinitionProvider
	{
		TArray<TObjectPtr<URockItemDefinition>> Definitions;
		FString Source;

		virtual void EnumerateItemDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const override
		{
			for (URockItemDefinition* Definition : Definitions)
			{
				OutDefinitions.Add(Definition);
			}
		}
		virtual FString DescribeItemSource(const URockItemDefinition*) const override { return Source; }
	};

	URockItemDefinition* MakeDefinition(FName ItemId)
	{
		URockItemDefinition* Definition = NewObject<URockItemDefinition>(GetTransientPackage());
		Definition->ItemId = ItemId;
		return Definition;
	}
}

// The registry is an engine subsystem: these tests run with no game world, which is the case that used to crash.
// Real item definitions come from the project's assets, so the tests check behavior that holds for any content.
TEST_CLASS(RockInventoryItemRegistryTests, "BRS.RockInventory.ItemRegistry")
{
	TEST_METHOD(GetInstance_WithoutGameWorld_ReturnsRegistry)
	{
		ASSERT_THAT(IsNotNull(URockItemRegistrySubsystem::GetInstance()));
	}

	TEST_METHOD(GetInstance_CalledTwice_ReturnsSameObject)
	{
		ASSERT_THAT(AreEqual(URockItemRegistrySubsystem::GetInstance(), URockItemRegistrySubsystem::GetInstance()));
	}

	TEST_METHOD(FindDefinition_NoneId_ReturnsNullAndWarns)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("None ItemID"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsNull(URockItemRegistrySubsystem::GetInstance()->FindDefinition(NAME_None)));
	}

	TEST_METHOD(FindDefinition_UnknownId_ReturnsNullAndWarns)
	{
		TestRunner->AddExpectedMessagePlain(TEXT("Could not find Item Definition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsNull(URockItemRegistrySubsystem::GetInstance()->FindDefinition(TEXT("Test.NoSuchItem.DoesNotExist")) ));
	}

	TEST_METHOD(GetAllDefinitions_EveryEntryIsFoundByItsOwnId)
	{
		TArray<URockItemDefinition*> All;
		URockItemRegistrySubsystem::GetInstance()->GetAllDefinitions(All);
		for (URockItemDefinition* Definition : All)
		{
			ASSERT_THAT(IsNotNull(Definition));
			ASSERT_THAT(AreEqual(Definition, URockItemStackLibrary::GetItemDefinitionById(Definition->ItemId)));
		}
	}

	TEST_METHOD(MarkDirty_Refresh_KeepsExistingDefinitions)
	{
		URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance();
		TArray<URockItemDefinition*> Before;
		Registry->GetAllDefinitions(Before);

		Registry->MarkDirty();

		TArray<URockItemDefinition*> After;
		Registry->GetAllDefinitions(After);

		// Nothing changed on disk, so the refresh must hand back the very same objects.
		ASSERT_THAT(AreEqual(Before.Num(), After.Num()));
		for (URockItemDefinition* Definition : Before)
		{
			ASSERT_THAT(IsTrue(After.Contains(Definition)));
		}
	}
};

// Providers: definitions that come from somewhere other than the primary asset scan.
TEST_CLASS(RockInventoryItemRegistryProviderTests, "BRS.RockInventory.ItemRegistry.Providers")
{
	using FTestProvider = RockInventoryItemRegistryTestsPrivate::FTestProvider;

	URockItemRegistrySubsystem* Registry = nullptr;
	FTestProvider Provider;
	FTestProvider Second;

	BEFORE_EACH()
	{
		Registry = URockItemRegistrySubsystem::GetInstance();
		Provider.Source = TEXT("ProviderA/Record.jsonc");
		Second.Source = TEXT("ProviderB/Record.jsonc");
	}

	AFTER_EACH()
	{
		if (Registry)
		{
			Registry->UnregisterProvider(&Provider);
			Registry->UnregisterProvider(&Second);
		}
	}

	TEST_METHOD(RegisterProvider_DefinitionIsFoundById)
	{
		URockItemDefinition* Definition = RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Added"));
		Provider.Definitions.Add(Definition);
		Registry->RegisterProvider(&Provider);

		ASSERT_THAT(AreEqual(Definition, Registry->FindDefinition(TEXT("Test.Provider.Added"))));
		TArray<URockItemDefinition*> All;
		Registry->GetAllDefinitions(All);
		ASSERT_THAT(IsTrue(All.Contains(Definition)));
	}

	TEST_METHOD(RegisterProvider_Twice_OffersDefinitionOnce)
	{
		URockItemDefinition* Definition = RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Twice"));
		Provider.Definitions.Add(Definition);
		Registry->RegisterProvider(&Provider);
		Registry->RegisterProvider(&Provider);

		TArray<URockItemDefinition*> All;
		Registry->GetAllDefinitions(All);
		ASSERT_THAT(AreEqual(1, All.FilterByPredicate([Definition](const URockItemDefinition* D) { return D == Definition; }).Num()));
	}

	TEST_METHOD(UnregisterProvider_DefinitionIsGone)
	{
		Provider.Definitions.Add(RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Removed")));
		Registry->RegisterProvider(&Provider);
		ASSERT_THAT(IsNotNull(Registry->FindDefinition(TEXT("Test.Provider.Removed"))));

		Registry->UnregisterProvider(&Provider);

		TestRunner->AddExpectedMessagePlain(TEXT("Could not find Item Definition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsNull(Registry->FindDefinition(TEXT("Test.Provider.Removed"))));
	}

	TEST_METHOD(ProviderRemovesDefinition_AfterNotify_DefinitionIsGone)
	{
		Provider.Definitions.Add(RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Dropped")));
		Registry->RegisterProvider(&Provider);
		ASSERT_THAT(IsNotNull(Registry->FindDefinition(TEXT("Test.Provider.Dropped"))));

		Provider.Definitions.Empty();
		Registry->NotifyProviderChanged();

		TestRunner->AddExpectedMessagePlain(TEXT("Could not find Item Definition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsNull(Registry->FindDefinition(TEXT("Test.Provider.Dropped"))));
	}

	TEST_METHOD(DuplicateIdAcrossProviders_ErrorNamesBothSources_FirstWins)
	{
		URockItemDefinition* First = RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Duplicate"));
		URockItemDefinition* Later = RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Duplicate"));
		Provider.Definitions.Add(First);
		Second.Definitions.Add(Later);
		Registry->RegisterProvider(&Provider);
		Registry->RegisterProvider(&Second);

		// One log line names the ignored source, then the one that was kept.
		TestRunner->AddExpectedMessagePlain(TEXT("Duplicate ItemId 'Test.Provider.Duplicate' found! 'ProviderB/Record.jsonc' conflicts with existing 'ProviderA/Record.jsonc'"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(AreEqual(First, Registry->FindDefinition(TEXT("Test.Provider.Duplicate"))));
	}

	TEST_METHOD(DuplicateIdWithAsset_AssetWinsAndErrorNamesAssetPath)
	{
		TArray<URockItemDefinition*> Assets;
		Registry->GetAllDefinitions(Assets);
		ASSERT_THAT(IsTrue(Assets.Num() > 0));
		URockItemDefinition* Asset = Assets[0];
		const FString AssetPath = GetPathNameSafe(Asset);

		URockItemDefinition* Impostor = RockInventoryItemRegistryTestsPrivate::MakeDefinition(Asset->ItemId);
		Provider.Definitions.Add(Impostor);
		Registry->RegisterProvider(&Provider);

		TestRunner->AddExpectedMessagePlain(FString::Printf(TEXT("'ProviderA/Record.jsonc' conflicts with existing '%s'"), *AssetPath), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(AreEqual(Asset, Registry->FindDefinition(Asset->ItemId)));
		ASSERT_THAT(AreEqual(AssetPath, GetPathNameSafe(Registry->FindDefinition(Asset->ItemId))));
	}

	TEST_METHOD(InPlaceUpdate_KeepsPointerAndRebuildsIdMap)
	{
		URockItemDefinition* Definition = RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Before"));
		Provider.Definitions.Add(Definition);
		Registry->RegisterProvider(&Provider);
		ASSERT_THAT(AreEqual(Definition, Registry->FindDefinition(TEXT("Test.Provider.Before"))));

		Definition->ItemId = TEXT("Test.Provider.After");
		Registry->NotifyProviderChanged();

		ASSERT_THAT(AreEqual(Definition, Registry->FindDefinition(TEXT("Test.Provider.After"))));
		TestRunner->AddExpectedMessagePlain(TEXT("Could not find Item Definition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsNull(Registry->FindDefinition(TEXT("Test.Provider.Before"))));
	}

	TEST_METHOD(ProviderAdded_LeavesAssetDefinitionsUnchanged)
	{
		TArray<URockItemDefinition*> Before;
		Registry->GetAllDefinitions(Before);

		Provider.Definitions.Add(RockInventoryItemRegistryTestsPrivate::MakeDefinition(TEXT("Test.Provider.Extra")));
		Registry->RegisterProvider(&Provider);

		for (URockItemDefinition* Asset : Before)
		{
			ASSERT_THAT(AreEqual(Asset, Registry->FindDefinition(Asset->ItemId)));
		}
		TArray<URockItemDefinition*> After;
		Registry->GetAllDefinitions(After);
		ASSERT_THAT(AreEqual(Before.Num() + 1, After.Num()));
	}

	TEST_METHOD(ProviderOffersNoneId_Skipped)
	{
		Provider.Definitions.Add(RockInventoryItemRegistryTestsPrivate::MakeDefinition(NAME_None));
		Registry->RegisterProvider(&Provider);

		TestRunner->AddExpectedMessagePlain(TEXT("has a None ItemId"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		TArray<URockItemDefinition*> All;
		Registry->GetAllDefinitions(All);
		ASSERT_THAT(IsFalse(All.ContainsByPredicate([](const URockItemDefinition* D) { return D->ItemId.IsNone(); })));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
