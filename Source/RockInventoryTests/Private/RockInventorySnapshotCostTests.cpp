// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Inventory/RockInventoryData.h"
#include "Library/RockInventoryLibrary.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// T-128 (measure only): what the whole-inventory snapshot (FRockInventoryData::FromInventory) costs inside MoveItem, AddItemToSlot and
// RemoveMatching. The element counts are exact and asserted; the times are logged as "RockInventory snapshot cost:" lines (wall clock,
// machine and build dependent, never asserted). Numbers are recorded in Docs/RockInventory/DevNotes.md.
TEST_CLASS(RockInventorySnapshotCostTests, "BRS.RockInventory.SnapshotCost")
{
	FRockInventoryFixture Fixture;

	static constexpr int32 Rounds = 10;
	static constexpr int32 PerRound = 20;

	/** Every fourth cell holds a 1x1 item (a quarter of the grid is occupied), the rest of the first column stays free. */
	void Fill(int32 Side, URockItemDefinition* Filler)
	{
		Fixture.InitGrid(Side, Side);
		for (int32 Index = 0; Index < Side * Side; Index += 4)
		{
			Fixture.PlaceAt(Filler, 1, Fixture.SlotAt(Index % Side, Index / Side));
		}
	}

	static double PerCallMicroseconds(double Seconds, int32 Calls) { return Seconds * 1.0e6 / Calls; }

	void MeasureSide(int32 Side)
	{
		URockItemDefinition* Filler = Fixture.MakeDefinition("Filler");
		URockItemDefinition* Probe = Fixture.MakeDefinition("Probe");
		Fill(Side, Filler);
		const int32 NumSlots = Side * Side;
		const int32 NumStacks = Fixture.Inventory->GetNumItemStacks();

		// The copy alone
		const FRockInventoryData Snapshot = FRockInventoryData::FromInventory(Fixture.Inventory);
		ASSERT_THAT(AreEqual(NumSlots, Snapshot.Slots.Num()));
		ASSERT_THAT(IsTrue(Snapshot.Stacks.Num() >= NumStacks));

		double Seconds = FPlatformTime::Seconds();
		for (int32 Index = 0; Index < Rounds * PerRound; ++Index)
		{
			const FRockInventoryData Copy = FRockInventoryData::FromInventory(Fixture.Inventory);
			ASSERT_THAT(IsTrue(Copy.Slots.Num() == NumSlots));
		}
		const double CopyUs = PerCallMicroseconds(FPlatformTime::Seconds() - Seconds, Rounds * PerRound);

		// MoveItem: one item out to a free cell and back
		const FRockInventorySlotHandle From = Fixture.SlotAt(0, 0);
		const FRockInventorySlotHandle To = Fixture.SlotAt(1, 0);
		int32 Moves = 0;
		Seconds = FPlatformTime::Seconds();
		for (int32 Index = 0; Index < Rounds * PerRound; ++Index)
		{
			const bool bOut = Index % 2 == 0;
			Moves += URockInventoryLibrary::MoveItem(Fixture.Inventory, bOut ? From : To, Fixture.Inventory, bOut ? To : From) ? 1 : 0;
		}
		const double MoveUs = PerCallMicroseconds(FPlatformTime::Seconds() - Seconds, Rounds * PerRound);
		ASSERT_THAT(AreEqual(Rounds * PerRound, Moves));

		// AddItemToSlot into free cells, then RemoveMatching them again
		double AddSeconds = 0.0;
		double RemoveSeconds = 0.0;
		int32 Added = 0;
		int32 Removed = 0;
		for (int32 Round = 0; Round < Rounds; ++Round)
		{
			double Start = FPlatformTime::Seconds();
			for (int32 Index = 0; Index < PerRound; ++Index)
			{
				int32 OutAdded = 0;
				const int32 Cell = 1 + Index * 4;
				const ERockAddRefusal Refusal = URockInventoryLibrary::AddItemToSlot(
					Fixture.Inventory, Fixture.SlotAt(Cell % Side, Cell / Side), FRockItemStack(Probe, 1), ERockItemOrientation::Horizontal, OutAdded);
				Added += Refusal == ERockAddRefusal::None ? OutAdded : 0;
			}
			AddSeconds += FPlatformTime::Seconds() - Start;

			Start = FPlatformTime::Seconds();
			for (int32 Index = 0; Index < PerRound; ++Index)
			{
				Removed += URockInventoryLibrary::RemoveMatching(
					Fixture.Inventory, [](const FRockItemStack& Stack) { return Stack.GetItemId() == FName("Probe"); }, 1, true);
			}
			RemoveSeconds += FPlatformTime::Seconds() - Start;
		}
		ASSERT_THAT(AreEqual(Rounds * PerRound, Added));
		ASSERT_THAT(AreEqual(Rounds * PerRound, Removed));

		UE_LOG(LogTemp, Display,
			TEXT("RockInventory snapshot cost: %d slots, %d stacks: FromInventory %.1f us; MoveItem %.1f us; AddItemToSlot %.1f us; RemoveMatching %.1f us (per call, mean of %d)"),
			NumSlots, NumStacks, CopyUs, MoveUs, PerCallMicroseconds(AddSeconds, Rounds * PerRound),
			PerCallMicroseconds(RemoveSeconds, Rounds * PerRound), Rounds * PerRound);
	}

	TEST_METHOD(Snapshot_HundredSlots_IsLogged) { MeasureSide(10); }
	TEST_METHOD(Snapshot_FourHundredSlots_IsLogged) { MeasureSide(20); }
	TEST_METHOD(Snapshot_SixteenHundredSlots_IsLogged) { MeasureSide(40); }
};

#endif // WITH_DEV_AUTOMATION_TESTS
