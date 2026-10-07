// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Access/RockInventoryAccessSubsystem.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Inventory/RockInventoryConfig.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// The server commands of URockInventoryManagerComponent must not trust the client: the instigator comes from the owning
// connection, and every inventory a command names goes through CanAccess. The Server_*_Implementation functions are called
// directly, which is the code the RPC runs on the server (a test world has authority).
TEST_CLASS(RockInventoryServerCommandTests, "BRS.RockInventory.ServerCommands")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	// The sender: a controller that owns a pawn and the manager component, next to its own inventory.
	APawn* AttackerPawn = nullptr;
	AController* Attacker = nullptr;
	URockInventoryManagerComponent* Manager = nullptr;
	URockInventory* Mine = nullptr;
	URockItemDefinition* Apple = nullptr;
	URockInventoryAccessSubsystem* Access = nullptr;

	URockInventory* MakeInventory(AActor* InOwner)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 1, ERockItemSizePolicy::RespectSize)};
		URockInventory* Inventory = NewObject<URockInventory>(InOwner);
		Inventory->Owner = InOwner;
		Inventory->Init(Config);
		KeepAlive.Emplace(Config);
		KeepAlive.Emplace(Inventory);
		return Inventory;
	}

	AController& MakeControllerFor(APawn& Pawn)
	{
		AController& Controller = Fixture.Spawner.SpawnActor<APlayerController>();
		Controller.SetPawn(&Pawn);
		return Controller;
	}

	BEFORE_EACH()
	{
		AttackerPawn = &SpawnPawnAt(Fixture.Spawner, FVector::ZeroVector);
		Attacker = &MakeControllerFor(*AttackerPawn);
		Manager = NewObject<URockInventoryManagerComponent>(Attacker);
		KeepAlive.Emplace(Manager);
		Mine = MakeInventory(AttackerPawn);
		Apple = Fixture.MakeDefinition("Apple");
		Access = URockInventoryAccessSubsystem::Get(AttackerPawn);
		ASSERT_THAT(IsNotNull(Access));
	}

	FRockInventorySlotHandle Slot(const URockInventory* Inventory, int32 Column) const
	{
		return FRockInventorySlotHandle(Inventory->GetSectionInfo(RockInventoryTags::Inventory_Section_Backpack).GetFirstSlotIndex() + Column);
	}

	FRockItemStackHandle Place(URockInventory* Inventory, int32 Column)
	{
		const FRockItemStackHandle Handle = Inventory->AddItemToInventory(FRockItemStack(Apple, 1));
		FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(Slot(Inventory, Column));
		Entry.ItemHandle = Handle;
		Inventory->SetSlotByHandle(Slot(Inventory, Column), Entry);
		return Handle;
	}

	bool HasItem(const URockInventory* Inventory, int32 Column) const
	{
		return Inventory->GetSlotByHandle(Slot(Inventory, Column)).ItemHandle.IsValid();
	}

	/** An inventory on another actor, `Distance` cm from the attacker's pawn. */
	URockInventory* MakeInventoryAt(float Distance)
	{
		return MakeInventory(&SpawnPawnAt(Fixture.Spawner, FVector(Distance, 0.f, 0.f)));
	}

	FRockMoveItemTransaction MoveCommand(URockInventory* From, URockInventory* To, AController* ClaimedInstigator)
	{
		return FRockMoveItemTransaction(ClaimedInstigator, From, Slot(From, 0), To, Slot(To, 1));
	}

	TEST_METHOD(Move_WithinTheSendersOwnInventory_Executes)
	{
		Place(Mine, 0);

		Manager->Server_MoveItem_Implementation(MoveCommand(Mine, Mine, Attacker));

		ASSERT_THAT(IsFalse(HasItem(Mine, 0)));
		ASSERT_THAT(IsTrue(HasItem(Mine, 1)));
	}

	TEST_METHOD(Move_OutOfAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		Place(Victim, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(MoveCommand(Victim, Mine, Attacker));

		ASSERT_THAT(IsTrue(HasItem(Victim, 0)));
		ASSERT_THAT(IsFalse(HasItem(Mine, 1)));
	}

	TEST_METHOD(Move_IntoAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		Place(Mine, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(MoveCommand(Mine, Victim, Attacker));

		ASSERT_THAT(IsTrue(HasItem(Mine, 0)));
		ASSERT_THAT(IsFalse(HasItem(Victim, 1)));
	}

	TEST_METHOD(Move_FromAContainerWithinReach_Executes)
	{
		URockInventory* Chest = MakeInventoryAt(100.f);
		Place(Chest, 0);
		ASSERT_THAT(IsTrue(Access->Open(Attacker, Chest) == ERockOpenResult::Opened));

		Manager->Server_MoveItem_Implementation(MoveCommand(Chest, Mine, Attacker));

		ASSERT_THAT(IsFalse(HasItem(Chest, 0)));
		ASSERT_THAT(IsTrue(HasItem(Mine, 1)));
	}

	TEST_METHOD(Move_ReachIsTheRegistrySetting)
	{
		URockInventory* Chest = MakeInventoryAt(100.f);
		Place(Chest, 0);
		ASSERT_THAT(IsTrue(Access->Open(Attacker, Chest) == ERockOpenResult::Opened));
		Access->DefaultReach = 50.f;
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(MoveCommand(Chest, Mine, Attacker));

		ASSERT_THAT(IsTrue(HasItem(Chest, 0)));
	}

	TEST_METHOD(Move_FromAContainerWithinReachThatWasNeverOpened_IsRefused)
	{
		URockInventory* Chest = MakeInventoryAt(100.f);
		Place(Chest, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(MoveCommand(Chest, Mine, Attacker));

		ASSERT_THAT(IsTrue(HasItem(Chest, 0)));
		ASSERT_THAT(IsFalse(HasItem(Mine, 1)));
	}

	TEST_METHOD(Move_WithASpoofedInstigator_ActsAsTheOwningController)
	{
		// The victim holds a drag lock on the sender's slot. A client that names the victim as instigator would walk past it.
		AController& Victim = MakeControllerFor(SpawnPawnAt(Fixture.Spawner, FVector(5000.f, 0.f, 0.f)));
		Place(Mine, 0);
		Mine->RegisterSlotStatus(&Victim, Slot(Mine, 0), ERockSlotStatus::Pending);
		TestRunner->AddExpectedMessagePlain(TEXT("Source slot is locked by other"), ELogVerbosity::Warning);

		Manager->Server_MoveItem_Implementation(MoveCommand(Mine, Mine, &Victim));

		ASSERT_THAT(IsTrue(HasItem(Mine, 0)));
		ASSERT_THAT(IsFalse(HasItem(Mine, 1)));
	}

	TEST_METHOD(Authorize_ReplacesTheClientInstigatorWithTheOwningController)
	{
		AController& Other = MakeControllerFor(SpawnPawnAt(Fixture.Spawner, FVector(5000.f, 0.f, 0.f)));
		FRockMoveItemTransaction Command = MoveCommand(Mine, Mine, &Other);

		ASSERT_THAT(IsTrue(Manager->AuthorizeServerCommand(Command, {Command.SourceInventory, Command.TargetInventory})));

		ASSERT_THAT(AreEqual(Attacker, Command.Instigator.Get()));
	}

	TEST_METHOD(Authorize_WithoutAnOwningController_IsRefused)
	{
		AActor& Stray = Fixture.Spawner.SpawnActor<AActor>();
		URockInventoryManagerComponent* Orphan = NewObject<URockInventoryManagerComponent>(&Stray);
		KeepAlive.Emplace(Orphan);
		FRockMoveItemTransaction Command = MoveCommand(Mine, Mine, Attacker);
		TestRunner->AddExpectedMessagePlain(TEXT("has no owning controller"), ELogVerbosity::Warning);

		ASSERT_THAT(IsFalse(Orphan->AuthorizeServerCommand(Command, {Command.SourceInventory})));
	}

	TEST_METHOD(Authorize_WithANullInventory_IsRefused)
	{
		FRockMoveItemTransaction Command = MoveCommand(Mine, Mine, Attacker);
		Command.TargetInventory = nullptr;
		TestRunner->AddExpectedMessagePlain(TEXT("null inventory"), ELogVerbosity::Warning);

		ASSERT_THAT(IsFalse(Manager->AuthorizeServerCommand(Command, {Command.SourceInventory, Command.TargetInventory})));
	}

	TEST_METHOD(CanAccess_OwnPawnAndController_AreAllowedAtAnyDistance)
	{
		URockInventory* OnController = MakeInventory(Attacker);
		AttackerPawn->SetActorLocation(FVector(9000.f, 0.f, 0.f));

		ASSERT_THAT(IsTrue(Manager->CanAccess(OnController, Attacker)));
		ASSERT_THAT(IsTrue(Manager->CanAccess(Mine, Attacker)));
	}

	TEST_METHOD(Authorize_LootIntoAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		FRockLootWorldItemTransaction Command;
		Command.TargetInventory = Victim;
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		ASSERT_THAT(IsFalse(Manager->AuthorizeServerCommand(Command, {Command.TargetInventory})));
	}

	TEST_METHOD(Drop_FromAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		Place(Victim, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_DropItem_Implementation(FRockDropItemTransaction(Attacker, Victim, Slot(Victim, 0)));

		ASSERT_THAT(IsTrue(HasItem(Victim, 0)));
	}

	TEST_METHOD(SlotStatus_IsClaimedForTheOwningController)
	{
		Place(Mine, 0);

		Manager->Server_RegisterSlotStatus_Implementation(Mine, Slot(Mine, 0), ERockSlotStatus::Pending);

		ASSERT_THAT(AreEqual(Attacker, Mine->GetPendingSlotState(Slot(Mine, 0)).Controller.Get()));
	}

	TEST_METHOD(SlotStatus_OnAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		Place(Victim, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_RegisterSlotStatus_Implementation(Victim, Slot(Victim, 0), ERockSlotStatus::Pending);

		ASSERT_THAT(AreEqual(ERockSlotStatus::Empty, Victim->GetSlotStatus(Slot(Victim, 0))));
	}

	TEST_METHOD(SlotStatus_ReleaseOnAnotherPlayersInventoryOutOfReach_IsRefused)
	{
		URockInventory* Victim = MakeInventoryAt(5000.f);
		Place(Victim, 0);
		AController& Owner = MakeControllerFor(SpawnPawnAt(Fixture.Spawner, FVector(5000.f, 0.f, 0.f)));
		Victim->RegisterSlotStatus(&Owner, Slot(Victim, 0), ERockSlotStatus::Pending);
		TestRunner->AddExpectedMessagePlain(TEXT("may not access inventory"), ELogVerbosity::Warning);

		Manager->Server_ReleaseSlotStatus_Implementation(Victim, Slot(Victim, 0));

		ASSERT_THAT(AreEqual(ERockSlotStatus::Pending, Victim->GetSlotStatus(Slot(Victim, 0))));
	}
};
#endif // WITH_DEV_AUTOMATION_TESTS
