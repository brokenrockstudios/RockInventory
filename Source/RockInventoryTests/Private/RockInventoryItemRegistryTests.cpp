// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Item/RockItemDefinition.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Library/RockItemStackLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
