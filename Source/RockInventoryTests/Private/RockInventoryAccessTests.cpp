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

// The access registry (T-75): who may touch which inventory. Deny by default; the reasons are Owner, Open, Proximity and
// Shared. The reach re-check is driven by hand (RecheckReach), the timer only calls it.
TEST_CLASS(RockInventoryAccessTests, "BRS.RockInventory.Access")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;

	APawn* PlayerPawn = nullptr;
	AController* Player = nullptr;
	URockInventory* Mine = nullptr;
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

	/** A player: a controller that possesses a pawn at Location. */
	AController& MakePlayerAt(const FVector& Location, APawn** OutPawn = nullptr)
	{
		APawn& Pawn = SpawnPawnAt(Fixture.Spawner, Location);
		AController& Controller = Fixture.Spawner.SpawnActor<APlayerController>();
		Controller.SetPawn(&Pawn);
		Pawn.Controller = &Controller;
		if (OutPawn)
		{
			*OutPawn = &Pawn;
		}
		return Controller;
	}

	/** A chest-like inventory: on an actor with no controller (also what a body or a dropped backpack is). */
	URockInventory* MakeChestAt(float Distance)
	{
		return MakeInventory(&SpawnPawnAt(Fixture.Spawner, FVector(Distance, 0.f, 0.f)));
	}

	bool Opened(const AController* Controller, const URockInventory* Inventory)
	{
		return Access->Open(Controller, Inventory) == ERockOpenResult::Opened;
	}

	BEFORE_EACH()
	{
		Player = &MakePlayerAt(FVector::ZeroVector, &PlayerPawn);
		Mine = MakeInventory(PlayerPawn);
		Access = URockInventoryAccessSubsystem::Get(PlayerPawn);
		ASSERT_THAT(IsNotNull(Access));
	}

	TEST_METHOD(Owner_OwnInventoryIsFullAtAnyDistance)
	{
		PlayerPawn->SetActorLocation(FVector(9000.f, 0.f, 0.f));
		URockInventory* OnController = MakeInventory(Player);

		const FRockInventoryAccess OnPawn = Access->GetAccess(Player, Mine);
		ASSERT_THAT(IsTrue(OnPawn.Rights == ERockInventoryRights::Full));
		ASSERT_THAT(IsTrue(OnPawn.Reasons == ERockAccessReason::Owner));
		ASSERT_THAT(IsTrue(Access->CanAccess(Player, OnController)));
	}

	TEST_METHOD(DenyByDefault_AnUnopenedChestInReachHasNoAccess)
	{
		URockInventory* Chest = MakeChestAt(100.f);

		ASSERT_THAT(IsTrue(Access->GetAccess(Player, Chest).Rights == ERockInventoryRights::None));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest, ERockInventoryRights::View)));
		ASSERT_THAT(IsFalse(Access->CanAccess(nullptr, Chest)));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, nullptr)));
	}

	TEST_METHOD(Open_ChestInReach_GivesFullAccessWithTheOpenReason)
	{
		URockInventory* Chest = MakeChestAt(100.f);

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));

		const FRockInventoryAccess Result = Access->GetAccess(Player, Chest);
		ASSERT_THAT(IsTrue(Result.Rights == ERockInventoryRights::Full));
		ASSERT_THAT(IsTrue(Result.Reasons == ERockAccessReason::Open));
		ASSERT_THAT(IsTrue(Access->IsOpen(Player, Chest)));
	}

	TEST_METHOD(Open_ChestOutOfReach_IsRefused)
	{
		URockInventory* Chest = MakeChestAt(5000.f);

		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::OutOfReach));
		ASSERT_THAT(IsFalse(Access->IsOpen(Player, Chest)));
	}

	TEST_METHOD(Open_AlreadyOpen_ReportsItAndDoesNotAnnounceAgain)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		int32 Announced = 0;
		Access->OnAfterOpen.AddLambda([&Announced](const AController&, const URockInventory&) { ++Announced; });

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::AlreadyOpen));

		ASSERT_THAT(AreEqual(1, Announced));
		ASSERT_THAT(AreEqual(1, Access->GetViewers(Chest).Num()));
	}

	TEST_METHOD(Open_AnotherPlayersInventory_IsRefusedEvenInReach)
	{
		APawn* OtherPawn = nullptr;
		MakePlayerAt(FVector(100.f, 0.f, 0.f), &OtherPawn);
		URockInventory* Theirs = MakeInventory(OtherPawn);

		ASSERT_THAT(IsTrue(Access->Open(Player, Theirs) == ERockOpenResult::NotOpenable));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Theirs)));
	}

	TEST_METHOD(Open_ABodyWithoutAControllerInReach_IsOpenToAnyone)
	{
		// A pawn nobody controls: what a dead player's body is.
		URockInventory* Body = MakeChestAt(150.f);

		ASSERT_THAT(IsTrue(Opened(Player, Body)));
	}

	TEST_METHOD(Open_OwnInventory_IsAllowedAndRecordedAtAnyDistance)
	{
		PlayerPawn->SetActorLocation(FVector(9000.f, 0.f, 0.f));
		URockInventory* OnController = MakeInventory(Player);

		ASSERT_THAT(IsTrue(Opened(Player, OnController)));
		Access->RecheckReach();

		ASSERT_THAT(IsTrue(Access->IsOpen(Player, OnController)));
	}

	TEST_METHOD(Open_NullArguments_AreInvalid)
	{
		URockInventory* Chest = MakeChestAt(100.f);

		ASSERT_THAT(IsTrue(Access->Open(nullptr, Chest) == ERockOpenResult::InvalidArguments));
		ASSERT_THAT(IsTrue(Access->Open(Player, nullptr) == ERockOpenResult::InvalidArguments));
	}

	TEST_METHOD(Close_RemovesTheAccessAndAnnouncesIt)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		Opened(Player, Chest);
		ERockCloseReason Reason = ERockCloseReason::OutOfReach;
		int32 Closed = 0;
		Access->OnClosed.AddLambda([&](const AController&, const URockInventory&, ERockCloseReason InReason) { ++Closed; Reason = InReason; });

		ASSERT_THAT(IsTrue(Access->Close(Player, Chest)));

		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest)));
		ASSERT_THAT(AreEqual(1, Closed));
		ASSERT_THAT(IsTrue(Reason == ERockCloseReason::Requested));
		ASSERT_THAT(IsFalse(Access->Close(Player, Chest)));
	}

	TEST_METHOD(Open_SeveralContainersAtOnce_AreAllOpenAndCloseAllEndsThem)
	{
		URockInventory* First = MakeChestAt(100.f);
		URockInventory* Second = MakeChestAt(200.f);
		Opened(Player, First);
		Opened(Player, Second);

		ASSERT_THAT(IsTrue(Access->CanAccess(Player, First)));
		ASSERT_THAT(IsTrue(Access->CanAccess(Player, Second)));
		ASSERT_THAT(AreEqual(2, Access->GetOpenInventories(Player).Num()));

		ASSERT_THAT(AreEqual(2, Access->CloseAll(Player)));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, First)));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Second)));
	}

	TEST_METHOD(Open_ByTwoPlayers_BothHaveAccessAndClosingOneKeepsTheOther)
	{
		AController& Other = MakePlayerAt(FVector(50.f, 0.f, 0.f));
		URockInventory* Chest = MakeChestAt(100.f);
		Opened(Player, Chest);
		Opened(&Other, Chest);

		ASSERT_THAT(AreEqual(2, Access->GetViewers(Chest).Num()));

		Access->Close(Player, Chest);
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest)));
		ASSERT_THAT(IsTrue(Access->CanAccess(&Other, Chest)));
	}

	TEST_METHOD(Open_ByOnePlayer_GivesAnotherNothing)
	{
		AController& Other = MakePlayerAt(FVector(50.f, 0.f, 0.f));
		URockInventory* Chest = MakeChestAt(100.f);
		Opened(Player, Chest);

		ASSERT_THAT(IsFalse(Access->CanAccess(&Other, Chest)));
	}

	TEST_METHOD(WalkingAway_LosesAccessAtOnceAndTheRecheckClosesTheOpen)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		Opened(Player, Chest);
		int32 Closed = 0;
		ERockCloseReason Reason = ERockCloseReason::Requested;
		Access->OnClosed.AddLambda([&](const AController&, const URockInventory&, ERockCloseReason InReason) { ++Closed; Reason = InReason; });

		PlayerPawn->SetActorLocation(FVector(5000.f, 0.f, 0.f));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest)));
		ASSERT_THAT(IsTrue(Access->IsOpen(Player, Chest)));

		Access->RecheckReach();
		ASSERT_THAT(IsFalse(Access->IsOpen(Player, Chest)));
		ASSERT_THAT(AreEqual(1, Closed));
		ASSERT_THAT(IsTrue(Reason == ERockCloseReason::OutOfReach));

		// Walking back does not reopen it: the player has to open it again.
		PlayerPawn->SetActorLocation(FVector(100.f, 0.f, 0.f));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest)));
		Access->RecheckReach();
		ASSERT_THAT(AreEqual(1, Closed));
	}

	TEST_METHOD(Tick_RechecksAfterTheInterval)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		Opened(Player, Chest);
		PlayerPawn->SetActorLocation(FVector(5000.f, 0.f, 0.f));

		Access->Tick(Access->RecheckInterval * 0.5f);
		ASSERT_THAT(IsTrue(Access->IsOpen(Player, Chest)));
		Access->Tick(Access->RecheckInterval);
		ASSERT_THAT(IsFalse(Access->IsOpen(Player, Chest)));
	}

	TEST_METHOD(Reach_DefaultReachIsTheRegistrySetting)
	{
		URockInventory* Chest = MakeChestAt(300.f);
		Access->DefaultReach = 200.f;
		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::OutOfReach));

		Access->DefaultReach = 400.f;
		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
	}

	TEST_METHOD(Reach_PolicyOverrideReplacesTheDefault)
	{
		URockInventory* Chest = MakeChestAt(300.f);
		FRockInventoryAccessPolicy Policy;
		Policy.ReachOverride = 1000.f;
		Access->SetPolicy(Chest, Policy);

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
	}

	TEST_METHOD(Reach_IgnoreMode_StaysOpenAtAnyDistance)
	{
		URockInventory* Chest = MakeChestAt(50000.f);
		FRockInventoryAccessPolicy Policy;
		Policy.Reach = ERockReachMode::Ignore;
		Access->SetPolicy(Chest, Policy);

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
		Access->RecheckReach();

		ASSERT_THAT(IsTrue(Access->CanAccess(Player, Chest)));
	}

	TEST_METHOD(Reach_CustomPredicate_DecidesTheOpenAndTheRecheck)
	{
		URockInventory* Chest = MakeChestAt(5000.f);
		bool bAllow = false;
		FRockInventoryAccessPolicy Policy;
		Policy.Reach = ERockReachMode::Custom;
		Policy.CustomReach = [&bAllow](const AController&, const URockInventory&) { return bAllow; };
		Access->SetPolicy(Chest, Policy);

		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::OutOfReach));

		bAllow = true;
		ASSERT_THAT(IsTrue(Opened(Player, Chest)));

		bAllow = false;
		Access->RecheckReach();
		ASSERT_THAT(IsFalse(Access->IsOpen(Player, Chest)));
	}

	TEST_METHOD(Reach_CustomModeWithoutAPredicate_Refuses)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		FRockInventoryAccessPolicy Policy;
		Policy.Reach = ERockReachMode::Custom;
		Access->SetPolicy(Chest, Policy);

		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::OutOfReach));
	}

	TEST_METHOD(Policy_OwnerOnly_RefusesOthersAndAllowsTheOwner)
	{
		AController& Other = MakePlayerAt(FVector(50.f, 0.f, 0.f));
		URockInventory* Chest = MakeChestAt(100.f);
		FRockInventoryAccessPolicy Policy;
		Policy.OpenableBy = ERockOpenableBy::OwnerOnly;
		Policy.Owner = Player;
		Access->SetPolicy(Chest, Policy);

		ASSERT_THAT(IsTrue(Access->Open(&Other, Chest) == ERockOpenResult::NotOpenable));
		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
	}

	TEST_METHOD(Policy_AnyoneOpensAnotherPlayersInventory)
	{
		APawn* OtherPawn = nullptr;
		MakePlayerAt(FVector(100.f, 0.f, 0.f), &OtherPawn);
		URockInventory* Theirs = MakeInventory(OtherPawn);
		FRockInventoryAccessPolicy Policy;
		Policy.OpenableBy = ERockOpenableBy::Anyone;
		Access->SetPolicy(Theirs, Policy);

		ASSERT_THAT(IsTrue(Opened(Player, Theirs)));
	}

	TEST_METHOD(Policy_ClearedPolicyFallsBackToTheDefaults)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		FRockInventoryAccessPolicy Policy;
		Policy.OpenableBy = ERockOpenableBy::OwnerOnly;
		Access->SetPolicy(Chest, Policy);
		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::NotOpenable));

		Access->ClearPolicy(Chest);

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
	}

	TEST_METHOD(BeforeOpen_AVetoRefusesTheOpen)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		int32 Announced = 0;
		Access->OnBeforeOpen.AddLambda([Chest](FRockInventoryOpenRequest& Request) { Request.bVeto = Request.Inventory == Chest; });
		Access->OnAfterOpen.AddLambda([&Announced](const AController&, const URockInventory&) { ++Announced; });

		ASSERT_THAT(IsTrue(Access->Open(Player, Chest) == ERockOpenResult::Vetoed));

		ASSERT_THAT(IsFalse(Access->IsOpen(Player, Chest)));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest)));
		ASSERT_THAT(AreEqual(0, Announced));
	}

	TEST_METHOD(BeforeOpen_SeesTheRequesterAndNoVetoOpens)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		const AController* Seen = nullptr;
		Access->OnBeforeOpen.AddLambda([&Seen](FRockInventoryOpenRequest& Request) { Seen = Request.Controller; });

		ASSERT_THAT(IsTrue(Opened(Player, Chest)));
		ASSERT_THAT(AreEqual(static_cast<const AController*>(Player), Seen));
	}

	TEST_METHOD(AfterOpen_AnnouncesWhoOpenedWhat)
	{
		URockInventory* Chest = MakeChestAt(100.f);
		const AController* Who = nullptr;
		const URockInventory* What = nullptr;
		Access->OnAfterOpen.AddLambda([&](const AController& Controller, const URockInventory& Inventory) { Who = &Controller; What = &Inventory; });

		Opened(Player, Chest);

		ASSERT_THAT(AreEqual(static_cast<const AController*>(Player), Who));
		ASSERT_THAT(AreEqual(static_cast<const URockInventory*>(Chest), What));
	}

	TEST_METHOD(Proximity_OwnedContainerNearby_IsViewOnlyUntilOpened)
	{
		URockInventory* BaseChest = MakeChestAt(1500.f);
		FRockInventoryAccessPolicy Policy;
		Policy.Owner = Player;
		Access->SetPolicy(BaseChest, Policy);

		const FRockInventoryAccess Near = Access->GetAccess(Player, BaseChest);
		ASSERT_THAT(IsTrue(Near.Rights == ERockInventoryRights::View));
		ASSERT_THAT(IsTrue(Near.Reasons == ERockAccessReason::Proximity));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, BaseChest, ERockInventoryRights::Full)));
		ASSERT_THAT(IsTrue(Access->CanAccess(Player, BaseChest, ERockInventoryRights::View)));

		PlayerPawn->SetActorLocation(FVector(1300.f, 0.f, 0.f));
		ASSERT_THAT(IsTrue(Opened(Player, BaseChest)));
		const FRockInventoryAccess Both = Access->GetAccess(Player, BaseChest);
		ASSERT_THAT(IsTrue(Both.Rights == ERockInventoryRights::Full));
		ASSERT_THAT(IsTrue(EnumHasAllFlags(Both.Reasons, ERockAccessReason::Open | ERockAccessReason::Proximity)));
	}

	TEST_METHOD(Proximity_OutsideTheRadiusOrForAnotherPlayer_GivesNothing)
	{
		AController& Other = MakePlayerAt(FVector(1400.f, 0.f, 0.f));
		URockInventory* BaseChest = MakeChestAt(1500.f);
		FRockInventoryAccessPolicy Policy;
		Policy.Owner = Player;
		Policy.ProximityRadius = 1000.f;
		Access->SetPolicy(BaseChest, Policy);

		ASSERT_THAT(IsFalse(Access->CanAccess(Player, BaseChest, ERockInventoryRights::View)));
		ASSERT_THAT(IsFalse(Access->CanAccess(&Other, BaseChest, ERockInventoryRights::View)));
	}

	TEST_METHOD(Shared_AGrantGivesItsRightsUntilRevoked)
	{
		URockInventory* Chest = MakeChestAt(5000.f);

		Access->GrantShared(Chest, Player, ERockInventoryRights::LimitedTake);
		const FRockInventoryAccess Granted = Access->GetAccess(Player, Chest);
		ASSERT_THAT(IsTrue(Granted.Rights == ERockInventoryRights::LimitedTake));
		ASSERT_THAT(IsTrue(Granted.Reasons == ERockAccessReason::Shared));
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest, ERockInventoryRights::Full)));

		Access->RevokeShared(Chest, Player);
		ASSERT_THAT(IsFalse(Access->CanAccess(Player, Chest, ERockInventoryRights::View)));
	}

	TEST_METHOD(Rights_NoneIsNeverAllowed)
	{
		FRockInventoryAccess Nothing;
		FRockInventoryAccess Full;
		Full.Rights = ERockInventoryRights::Full;

		ASSERT_THAT(IsFalse(Nothing.Allows(ERockInventoryRights::View)));
		ASSERT_THAT(IsFalse(Full.Allows(ERockInventoryRights::None)));
		ASSERT_THAT(IsTrue(Full.Allows(ERockInventoryRights::LimitedTake)));
	}

	TEST_METHOD(Manager_ServerOpenAndClose_DriveTheRegistry)
	{
		URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(Player);
		KeepAlive.Emplace(Manager);
		URockInventory* Chest = MakeChestAt(100.f);

		Manager->Server_OpenInventory_Implementation(Chest);
		ASSERT_THAT(IsTrue(Manager->CanAccess(Chest, Player)));

		Manager->Server_CloseInventory_Implementation(Chest);
		ASSERT_THAT(IsFalse(Manager->CanAccess(Chest, Player)));
	}

	TEST_METHOD(Manager_ServerOpen_OfAnotherPlayersInventory_IsRefused)
	{
		URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(Player);
		KeepAlive.Emplace(Manager);
		APawn* OtherPawn = nullptr;
		MakePlayerAt(FVector(100.f, 0.f, 0.f), &OtherPawn);
		URockInventory* Theirs = MakeInventory(OtherPawn);
		TestRunner->AddExpectedMessagePlain(TEXT("may not open inventory"), ELogVerbosity::Warning);

		Manager->Server_OpenInventory_Implementation(Theirs);

		ASSERT_THAT(IsFalse(Manager->CanAccess(Theirs, Player)));
	}

	TEST_METHOD(Manager_CanAccess_WithTheRequiredRights)
	{
		URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(Player);
		KeepAlive.Emplace(Manager);
		URockInventory* Chest = MakeChestAt(5000.f);
		Access->GrantShared(Chest, Player, ERockInventoryRights::View);

		ASSERT_THAT(IsTrue(Manager->CanAccess(Chest, Player, ERockInventoryRights::View)));
		ASSERT_THAT(IsFalse(Manager->CanAccess(Chest, Player)));
	}
};
#endif // WITH_DEV_AUTOMATION_TESTS
