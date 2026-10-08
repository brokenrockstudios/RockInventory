// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTestFixture.h"

#include "Access/RockInventoryAccessSubsystem.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/RockInventoryConfig.h"
#include "Item/Fragment/RockItemFragment_NestedInventory.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "Net/Subsystems/NetworkSubsystem.h"
#include "Replication/RockInventoryReplication.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CQTest.h"

// Gated replication (T-76), server side: which net groups the inventory, its item instances and its nested inventories are in as the
// access registry's viewers change. What a client then receives is checked over a real connection in
// Network.RockInventory.Gating (RockInventoryNetworkGatingTests.cpp).
TEST_CLASS(RockInventoryGatingTests, "BRS.RockInventory.Gating")
{
	FRockInventoryFixture Fixture;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	URockInventoryAccessSubsystem* Access = nullptr;
	bool bGatingWasEnabled = true;

	BEFORE_EACH()
	{
		Access = URockInventoryAccessSubsystem::Get(&Fixture.Spawner.GetWorld());
		bGatingWasEnabled = RockInventoryReplication::IsGatingEnabled();
		Fixture.InitGrid(4, 4);
	}

	AFTER_EACH()
	{
		SetGating(bGatingWasEnabled);
	}

	static void SetGating(bool bEnabled)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("RockInventory.GatedReplication")))
		{
			Variable->Set(bEnabled ? 1 : 0, ECVF_SetByCode);
		}
	}

	/** A player: a controller that possesses a pawn at Location. */
	APlayerController& MakePlayerAt(const FVector& Location, APawn** OutPawn = nullptr)
	{
		APawn& Pawn = SpawnPawnAt(Fixture.Spawner, Location);
		APlayerController& Controller = Fixture.Spawner.SpawnActor<APlayerController>();
		Controller.SetPawn(&Pawn);
		Pawn.Controller = &Controller;
		if (OutPawn)
		{
			*OutPawn = &Pawn;
		}
		return Controller;
	}

	/** A container that is not on a player, 100 cm from the origin (inside default reach). */
	URockInventory* MakeChest()
	{
		AActor& Chest = SpawnPawnAt(Fixture.Spawner, FVector(100.f, 0.f, 0.f));
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 4)};
		URockInventory* Inventory = NewObject<URockInventory>(&Chest);
		Inventory->Owner = &Chest;
		Inventory->Init(Config);
		KeepAlive.Emplace(Config);
		KeepAlive.Emplace(Inventory);
		return Inventory;
	}

	/** An item definition whose runtime instance owns a 2x2 nested inventory. */
	URockItemDefinition* MakeBackpack(bool bFollowsParentViewers = false)
	{
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2)};
		Config->Visibility = bFollowsParentViewers ? ERockNestedVisibility::FollowsParent : ERockNestedVisibility::Separate;
		KeepAlive.Emplace(Config);

		URockItemDefinition* Backpack = Fixture.MakeDefinition("Backpack");
		Backpack->RuntimeInstanceClass = URockItemInstance::StaticClass();
		FRockItemFragment_NestedInventory Fragment;
		Fragment.InventoryConfig = Config;
		Backpack->Fragments.Add(FInstancedStruct::Make(Fragment));
		return Backpack;
	}

	/** An item definition with a plain runtime instance (no nested inventory). */
	URockItemDefinition* MakeInstanced(FName Id)
	{
		URockItemDefinition* Definition = Fixture.MakeDefinition(Id);
		Definition->RuntimeInstanceClass = URockItemInstance::StaticClass();
		return Definition;
	}

	/** Puts an item in the first free cell of the inventory and returns its runtime instance. */
	URockItemInstance* AddInstanced(URockInventory* Inventory, URockItemDefinition* Definition)
	{
		FRockLootParams Params;
		FRockLootResult Result;
		URockInventoryLibrary::LootItemToInventory(Inventory, FRockItemStack(Definition, 1), Params, Result);
		URockItemInstance* Found = nullptr;
		Inventory->ForEachItemStack([&Found](const FRockItemStack& Stack)
		{
			Found = Stack.GetRuntimeInstance();
			return false;
		});
		return Found;
	}

	TArray<FName> GroupsOf(UObject* Object)
	{
		UNetworkSubsystem* NetSubsystem = Fixture.Spawner.GetWorld().GetSubsystem<UNetworkSubsystem>();
		if (!NetSubsystem || !Object)
		{
			return {};
		}
		return TArray<FName>(NetSubsystem->GetNetConditionGroupManager().GetSubObjectNetConditionGroups(FObjectKey(Object)));
	}

	bool InGroup(UObject* Object, const APlayerController& Viewer)
	{
		return GroupsOf(Object).Contains(RockInventoryReplication::GetViewerGroup(Viewer));
	}

	bool OwnerOnly(UObject* Object)
	{
		const TArray<FName> Groups = GroupsOf(Object);
		return Groups.Num() == 1 && Groups[0] == UE::Net::NetGroupOwner;
	}

	TEST_METHOD(NewInventoryAndItsInstance_AreInTheOwnerGroupOnly)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		ASSERT_THAT(IsNotNull(Instance));

		ASSERT_THAT(IsTrue(OwnerOnly(Chest)));
		ASSERT_THAT(IsTrue(OwnerOnly(Instance)));
		ASSERT_THAT(IsTrue(Chest->GetOwningActor()->IsReplicatedSubObjectRegistered(Chest)));
		ASSERT_THAT(IsTrue(Chest->GetOwningActor()->IsReplicatedSubObjectRegistered(Instance)));
	}

	TEST_METHOD(Open_PutsTheInventoryAndItsInstancesInTheViewerGroup_CloseTakesThemOut)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(IsNotNull(Instance));

		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Chest, Viewer)));
		ASSERT_THAT(IsTrue(InGroup(Instance, Viewer)));

		ASSERT_THAT(IsTrue(Access->Close(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(OwnerOnly(Chest)));
		ASSERT_THAT(IsTrue(OwnerOnly(Instance)));
	}

	TEST_METHOD(DestroyedInstance_LeavesNoGroupMemberships)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(IsNotNull(Instance));
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Instance, Viewer)));

		// The group manager keys memberships by FObjectKey; BeginDestroy must remove them, not only the registration (T-166)
		Instance->ConditionalBeginDestroy();
		ASSERT_THAT(IsTrue(GroupsOf(Instance).IsEmpty()));
	}

	TEST_METHOD(ItemAddedWhileOpen_JoinsTheViewerGroup)
	{
		URockInventory* Chest = MakeChest();
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));

		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));

		ASSERT_THAT(IsNotNull(Instance));
		ASSERT_THAT(IsTrue(InGroup(Instance, Viewer)));
	}

	TEST_METHOD(TwoViewers_EachHasAGroup_ClosingOneKeepsTheOther)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		APlayerController& First = MakePlayerAt(FVector::ZeroVector);
		APlayerController& Second = MakePlayerAt(FVector(50.f, 0.f, 0.f));
		ASSERT_THAT(IsNotNull(Instance));
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&First, Chest)));
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Second, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Instance, First) && InGroup(Instance, Second)));
		ASSERT_THAT(IsTrue(RockInventoryReplication::GetViewerGroup(First) != RockInventoryReplication::GetViewerGroup(Second)));

		Access->Close(&First, Chest);

		ASSERT_THAT(IsFalse(InGroup(Chest, First)));
		ASSERT_THAT(IsFalse(InGroup(Instance, First)));
		ASSERT_THAT(IsTrue(InGroup(Chest, Second)));
		ASSERT_THAT(IsTrue(InGroup(Instance, Second)));
	}

	TEST_METHOD(OutOfReach_RecheckTakesTheViewerOut)
	{
		URockInventory* Chest = MakeChest();
		APawn* Pawn = nullptr;
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector, &Pawn);
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Chest, Viewer)));

		Pawn->SetActorLocation(FVector(5000.f, 0.f, 0.f));
		Access->RecheckReach();

		ASSERT_THAT(IsFalse(Access->IsOpen(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(OwnerOnly(Chest)));
	}

	TEST_METHOD(Proximity_OwnersBaseContainerIsViewedWhileNearAndNotWhenAway)
	{
		URockInventory* Chest = MakeChest();
		APawn* Pawn = nullptr;
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector, &Pawn);
		FRockInventoryAccessPolicy Policy;
		Policy.Owner = &Viewer;
		Access->SetPolicy(Chest, Policy);
		ASSERT_THAT(IsTrue(InGroup(Chest, Viewer)));
		ASSERT_THAT(IsFalse(Access->IsOpen(&Viewer, Chest)));

		Pawn->SetActorLocation(FVector(9000.f, 0.f, 0.f));
		Access->RecheckReach();
		ASSERT_THAT(IsTrue(OwnerOnly(Chest)));

		Pawn->SetActorLocation(FVector(200.f, 0.f, 0.f));
		Access->RecheckReach();
		ASSERT_THAT(IsTrue(InGroup(Chest, Viewer)));
	}

	TEST_METHOD(Shared_GrantWithViewRightsAddsTheViewer_RevokeRemovesIt)
	{
		URockInventory* Chest = MakeChest();
		APlayerController& Viewer = MakePlayerAt(FVector(9000.f, 0.f, 0.f));

		Access->GrantShared(Chest, &Viewer, ERockInventoryRights::View);
		ASSERT_THAT(IsTrue(InGroup(Chest, Viewer)));

		Access->RevokeShared(Chest, &Viewer);
		ASSERT_THAT(IsTrue(OwnerOnly(Chest)));
	}

	TEST_METHOD(NestedInventory_IsItsOwnUnit_TheBackpackItemFollowsTheOuterInventory)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* BackpackInstance = AddInstanced(Chest, MakeBackpack());
		ASSERT_THAT(IsNotNull(BackpackInstance));
		URockInventory* Nested = BackpackInstance->GetNestedInventory();
		ASSERT_THAT(IsNotNull(Nested));
		URockItemInstance* Inside = AddInstanced(Nested, MakeInstanced("Lantern"));
		ASSERT_THAT(IsNotNull(Inside));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);

		// Opening the chest shows the backpack item, not what is inside the backpack
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(BackpackInstance, Viewer)));
		ASSERT_THAT(IsTrue(OwnerOnly(Nested)));
		ASSERT_THAT(IsTrue(OwnerOnly(Inside)));

		// Opening the backpack shows its contents
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Nested)));
		ASSERT_THAT(IsTrue(InGroup(Nested, Viewer)));
		ASSERT_THAT(IsTrue(InGroup(Inside, Viewer)));

		// Closing the backpack hides the contents again but keeps the backpack item
		Access->Close(&Viewer, Nested);
		ASSERT_THAT(IsTrue(OwnerOnly(Nested)));
		ASSERT_THAT(IsTrue(OwnerOnly(Inside)));
		ASSERT_THAT(IsTrue(InGroup(BackpackInstance, Viewer)));
	}

	TEST_METHOD(NestedInventoryThatFollowsItsParent_IsSeenWithTheGun_AndHasTheGunsAccess)
	{
		URockInventory* Chest = MakeChest();
		URockItemInstance* GunInstance = AddInstanced(Chest, MakeBackpack(true));
		ASSERT_THAT(IsNotNull(GunInstance));
		URockInventory* Scope = GunInstance->GetNestedInventory();
		ASSERT_THAT(IsNotNull(Scope));
		URockItemInstance* ScopeItem = AddInstanced(Scope, MakeInstanced("Scope"));
		ASSERT_THAT(IsNotNull(ScopeItem));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(&Viewer);
		Manager->RegisterComponent();
		KeepAlive.Emplace(Manager);
		ASSERT_THAT(IsTrue(Scope->NestedVisibility == ERockNestedVisibility::FollowsParent));
		ASSERT_THAT(IsTrue(Scope->GetGatingRoot() == Chest));
		ASSERT_THAT(IsFalse(Access->CanAccess(&Viewer, Scope)));

		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));

		ASSERT_THAT(IsTrue(InGroup(GunInstance, Viewer)));
		ASSERT_THAT(IsTrue(InGroup(Scope, Viewer)));
		ASSERT_THAT(IsTrue(InGroup(ScopeItem, Viewer)));
		ASSERT_THAT(IsTrue(Access->CanAccess(&Viewer, Scope)));
		ASSERT_THAT(IsNotNull(Manager->FindObserved(Chest)));
		ASSERT_THAT(IsTrue(Manager->FindObserved(Scope) == nullptr));

		// An item added to the scope's inventory while open joins too
		URockItemInstance* LaterItem = AddInstanced(Scope, MakeInstanced("Lens"));
		ASSERT_THAT(IsNotNull(LaterItem));
		ASSERT_THAT(IsTrue(InGroup(LaterItem, Viewer)));

		Access->Close(&Viewer, Scope);
		ASSERT_THAT(IsFalse(Access->IsOpen(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(OwnerOnly(Scope)));
		ASSERT_THAT(IsTrue(OwnerOnly(ScopeItem)));
		ASSERT_THAT(IsTrue(OwnerOnly(GunInstance)));
	}

	TEST_METHOD(NestedInventoryThatFollowsItsParent_LeavesTheViewersWhenTheGunIsMovedToAnUnviewedInventory)
	{
		URockInventory* Chest = MakeChest();
		URockInventory* Other = MakeChest();
		URockItemInstance* GunInstance = AddInstanced(Chest, MakeBackpack(true));
		ASSERT_THAT(IsNotNull(GunInstance));
		URockInventory* Scope = GunInstance->GetNestedInventory();
		ASSERT_THAT(IsNotNull(Scope));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Scope, Viewer)));
		FRockInventorySlotHandle From;
		for (int32 Index = 0; Index < 16; ++Index)
		{
			if (Chest->GetSlotByHandle(FRockInventorySlotHandle(Index)).ItemHandle.IsValid())
			{
				From = FRockInventorySlotHandle(Index);
				break;
			}
		}
		ASSERT_THAT(IsTrue(From.IsValid()));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Chest, From, Other, FRockInventorySlotHandle(5))));

		ASSERT_THAT(IsTrue(Scope->GetGatingRoot() == Other));
		ASSERT_THAT(IsFalse(InGroup(Scope, Viewer)));
		ASSERT_THAT(IsFalse(InGroup(GunInstance, Viewer)));
	}

	TEST_METHOD(OwnerOnlyNestedInventory_IsNeverSeenOrOpenedByAnyoneButItsPlayer)
	{
		APawn* PlayerPawn = nullptr;
		APlayerController& Player = MakePlayerAt(FVector::ZeroVector, &PlayerPawn);
		APlayerController& Thief = MakePlayerAt(FVector(50.f, 0.f, 0.f));
		// The player's own inventory, holding a secure container with an item in it
		URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
		Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 4)};
		URockInventory* Mine = NewObject<URockInventory>(PlayerPawn);
		Mine->Owner = PlayerPawn;
		Mine->Init(Config);
		KeepAlive.Emplace(Config);
		KeepAlive.Emplace(Mine);
		URockItemDefinition* Secure = MakeBackpack();
		Secure->Fragments.Reset();
		URockInventoryConfig* SecureConfig = NewObject<URockInventoryConfig>(GetTransientPackage());
		SecureConfig->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2)};
		SecureConfig->Visibility = ERockNestedVisibility::OwnerOnly;
		KeepAlive.Emplace(SecureConfig);
		FRockItemFragment_NestedInventory Fragment;
		Fragment.InventoryConfig = SecureConfig;
		Secure->Fragments.Add(FInstancedStruct::Make(Fragment));
		URockItemInstance* SecureInstance = AddInstanced(Mine, Secure);
		ASSERT_THAT(IsNotNull(SecureInstance));
		URockInventory* Inside = SecureInstance->GetNestedInventory();
		ASSERT_THAT(IsNotNull(Inside));
		URockItemInstance* Stash = AddInstanced(Inside, MakeInstanced("Stash"));
		ASSERT_THAT(IsNotNull(Stash));

		// Nobody else may open it, share it or ever be a viewer, even with the parent's rights
		FRockInventoryAccessPolicy Open;
		Open.OpenableBy = ERockOpenableBy::Anyone;
		Open.Reach = ERockReachMode::Ignore;
		Access->SetPolicy(Inside, Open);
		ASSERT_THAT(AreEqual(ERockOpenResult::NotOpenable, Access->Open(&Thief, Inside)));
		Access->GrantShared(Inside, &Thief, ERockInventoryRights::Full);
		ASSERT_THAT(IsFalse(Access->CanAccess(&Thief, Inside, ERockInventoryRights::View)));
		ASSERT_THAT(IsTrue(Access->GetViewerControllers(Inside).IsEmpty()));
		ASSERT_THAT(IsTrue(OwnerOnly(Inside)));
		ASSERT_THAT(IsTrue(OwnerOnly(Stash)));

		// The player has full access to it
		ASSERT_THAT(IsTrue(Access->CanAccess(&Player, Inside)));
	}

	TEST_METHOD(MovingAnItemOutOfAViewedInventory_TheInstanceLeavesTheViewerGroup)
	{
		URockInventory* Chest = MakeChest();
		URockInventory* Other = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		ASSERT_THAT(IsNotNull(Instance));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		ASSERT_THAT(IsTrue(InGroup(Instance, Viewer)));
		FRockInventorySlotHandle From;
		for (int32 Index = 0; Index < 16; ++Index)
		{
			if (Chest->GetSlotByHandle(FRockInventorySlotHandle(Index)).ItemHandle.IsValid())
			{
				From = FRockInventorySlotHandle(Index);
				break;
			}
		}
		ASSERT_THAT(IsTrue(From.IsValid()));

		ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Chest, From, Other, FRockInventorySlotHandle(5))));

		ASSERT_THAT(IsFalse(InGroup(Instance, Viewer)));
		ASSERT_THAT(IsTrue(OwnerOnly(Instance)));
	}

	TEST_METHOD(ObservedList_FollowsOpenAndClose_WithTheRevisionOfTheGrant)
	{
		URockInventory* Chest = MakeChest();
		AddInstanced(Chest, MakeInstanced("Lantern"));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(&Viewer);
		Manager->RegisterComponent();
		KeepAlive.Emplace(Manager);

		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));
		const FRockObservedInventory* Entry = Manager->FindObserved(Chest);
		ASSERT_THAT(IsNotNull(Entry));
		ASSERT_THAT(AreEqual(Chest->GetRevision(), Entry->Revision));

		Access->Close(&Viewer, Chest);
		ASSERT_THAT(IsTrue(Manager->FindObserved(Chest) == nullptr));
	}

	TEST_METHOD(SyncState_OnTheServerIsAlwaysLive)
	{
		URockInventory* Chest = MakeChest();
		ASSERT_THAT(AreEqual(ERockInventorySyncState::Live, Chest->GetSyncState()));
	}

	TEST_METHOD(GatingOff_RegistersWithoutGroups)
	{
		SetGating(false);
		URockInventory* Chest = MakeChest();
		URockItemInstance* Instance = AddInstanced(Chest, MakeInstanced("Lantern"));
		ASSERT_THAT(IsNotNull(Instance));
		APlayerController& Viewer = MakePlayerAt(FVector::ZeroVector);
		ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(&Viewer, Chest)));

		ASSERT_THAT(IsTrue(GroupsOf(Chest).IsEmpty()));
		ASSERT_THAT(IsTrue(GroupsOf(Instance).IsEmpty()));
		ASSERT_THAT(IsTrue(Chest->GetOwningActor()->IsReplicatedSubObjectRegistered(Chest)));
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
