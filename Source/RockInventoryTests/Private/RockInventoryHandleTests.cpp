// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStackHandle.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Handles are the only way to refer to slots and stacks, so their packing rules are worth pinning down.
TEST_CLASS(RockInventoryHandleTests, "BRS.RockInventory.Handles")
{
	TEST_METHOD(ItemHandle_Default_IsInvalid)
	{
		ASSERT_THAT(IsFalse(FRockItemStackHandle().IsValid()));
		ASSERT_THAT(IsFalse(FRockItemStackHandle::Invalid().IsValid()));
	}

	TEST_METHOD(ItemHandle_Create_RoundTripsIndexAndGeneration)
	{
		const FRockItemStackHandle Handle = FRockItemStackHandle::Create(7, 3);
		ASSERT_THAT(IsTrue(Handle.IsValid()));
		ASSERT_THAT(AreEqual(7, Handle.GetIndex()));
		ASSERT_THAT(AreEqual(3, Handle.GetGeneration()));
	}

	TEST_METHOD(ItemHandle_Create_MasksGenerationThatDoesNotFitItsBits)
	{
		// Generation wraps by design. Extra bits must not bleed into the index half.
		const FRockItemStackHandle Handle = FRockItemStackHandle::Create(5, 0x10002);
		ASSERT_THAT(IsTrue(Handle.IsValid()));
		ASSERT_THAT(AreEqual(5, Handle.GetIndex()));
		ASSERT_THAT(AreEqual(2, Handle.GetGeneration()));
	}

	TEST_METHOD(ItemHandle_Create_IndexThatDoesNotFitIsInvalid)
	{
		// Masking would alias slot 5, so an oversized index must not produce a usable handle.
		ASSERT_THAT(IsFalse(FRockItemStackHandle::Create(0x10005, 0).IsValid()));
		ASSERT_THAT(IsTrue(FRockItemStackHandle::Create(0xFFFF, 0).IsValid()));
	}

	TEST_METHOD(ItemHandle_Reset_BecomesInvalid)
	{
		FRockItemStackHandle Handle = FRockItemStackHandle::Create(1, 1);
		Handle.Reset();
		ASSERT_THAT(IsFalse(Handle.IsValid()));
	}

	TEST_METHOD(ItemHandle_SameIndexDifferentGeneration_AreNotEqual)
	{
		// This inequality is what makes a stale handle detectable after its index is reused.
		const FRockItemStackHandle Old = FRockItemStackHandle::Create(4, 0);
		const FRockItemStackHandle Reused = FRockItemStackHandle::Create(4, 1);
		ASSERT_THAT(IsTrue(Old != Reused));
		ASSERT_THAT(IsTrue(Old == FRockItemStackHandle::Create(4, 0)));
		ASSERT_THAT(AreEqual(Old.GetIndex(), Reused.GetIndex()));
	}

	TEST_METHOD(ItemHandle_NextGeneration_IncrementsAndWrapsWithoutIssuingAllOnes)
	{
		ASSERT_THAT(AreEqual(1u, FRockItemStackHandle::NextGeneration(0)));
		ASSERT_THAT(AreEqual(0xFFFEu, FRockItemStackHandle::NextGeneration(0xFFFD)));
		// 0xFFFF is skipped, otherwise Create(0xFFFF, 0xFFFF) packs to INDEX_NONE.
		ASSERT_THAT(AreEqual(0u, FRockItemStackHandle::NextGeneration(0xFFFE)));
		ASSERT_THAT(AreEqual(0u, FRockItemStackHandle::NextGeneration(0xFFFF)));
	}

	TEST_METHOD(ItemHandle_AllOnesIndexAndGeneration_CollidesWithInvalid)
	{
		// Documents why NextGeneration skips 0xFFFF. Only reachable with a 65536-entry inventory.
		ASSERT_THAT(IsFalse(FRockItemStackHandle::Create(0xFFFF, 0xFFFF).IsValid()));
		ASSERT_THAT(IsTrue(FRockItemStackHandle::Create(0xFFFF, FRockItemStackHandle::NextGeneration(0xFFFD)).IsValid()));
	}

	TEST_METHOD(SlotHandle_Default_IsInvalid)
	{
		ASSERT_THAT(IsFalse(FRockInventorySlotHandle().IsValid()));
		ASSERT_THAT(IsFalse(FRockInventorySlotHandle::Invalid().IsValid()));
	}

	TEST_METHOD(SlotHandle_ZeroIndex_IsValid)
	{
		const FRockInventorySlotHandle Handle(0);
		ASSERT_THAT(IsTrue(Handle.IsValid()));
		ASSERT_THAT(AreEqual(0, Handle.GetAbsoluteIndex()));
	}

	TEST_METHOD(SlotHandle_Equality_ComparesAbsoluteIndex)
	{
		ASSERT_THAT(IsTrue(FRockInventorySlotHandle(3) == FRockInventorySlotHandle(3)));
		ASSERT_THAT(IsTrue(FRockInventorySlotHandle(3) != FRockInventorySlotHandle(4)));
		ASSERT_THAT(AreEqual(GetTypeHash(FRockInventorySlotHandle(3)), GetTypeHash(FRockInventorySlotHandle(3))));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
