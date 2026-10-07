// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryNetTestActor.h"

#include "Components/PIENetworkComponent.h"

#if ENABLE_PIE_NETWORK_TEST

#include "Access/RockInventoryAccessSubsystem.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Engine/NetConnection.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryConfig.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryTags.h"
#include "UObject/Package.h"

// T-76: gated replication over a real connection. Everything is registered through the plugin's own path (URockInventory::Init,
// URockItemInstance::SetOwningInventory), not by hand as in the T-68 spike; who may see what comes from URockInventoryAccessSubsystem.
// Client 0 owns the actor, client 1 is the second player. Same harness and run command as RockInventoryNetworkTests.cpp:
// `RunTests.ps1 -Filter Network.RockInventory -Network`. Findings are in Docs/RockInventory/DevNotes.md ("Gated replication (T-76)").
NETWORK_TEST_CLASS(RockInventoryNetGating, "Network.RockInventory.Gating")
{
	struct FState : public FBasePIENetworkComponentState
	{
		ARockInventoryNetTestActor* Actor = nullptr;
	};

	FPIENetworkComponent<FState> Network{TestRunner, TestCommandBuilder, bInitializing};

	static constexpr int32 OwnerClient = 0;
	static constexpr int32 ViewerClient = 1;
	static constexpr int32 SettleFrames = 30;

	FRockInventorySlotHandle StartSlot{0};
	FRockInventorySlotHandle SecondSlot{5};
	FRockInventorySlotHandle ThirdSlot{10};

	BEFORE_EACH()
	{
		for (const TCHAR* Noise : {TEXT("DedicatedServerClientId"), TEXT("DedicatedServerClientSecret"), TEXT("Unable to initialize EOS platform"),
			TEXT("errors.com.redpoint.eos.no_connection"), TEXT("BroadcastLoadedModulesUpdated"), TEXT("Unable to init online subsystem"),
			TEXT("has not signed into EOS"), TEXT("GetUniqueNetId called with invalid AccountId"), TEXT("Generating descriptor for struct"),
			TEXT("SendRPC ClientSetViewTarget")})
		{
			TestRunner->AddExpectedMessagePlain(Noise, EAutomationExpectedMessageFlags::Contains, -1);
		}

		FNetworkComponentBuilder<FState>()
			.WithClients(2)
			.AsDedicatedServer()
			.WithGameInstanceClass(UGameInstance::StaticClass())
			.WithGameMode(AGameModeBase::StaticClass())
			.Build(Network);
	}

	void WaitFrames(int32 Frames)
	{
		TSharedRef<int32> Counter = MakeShared<int32>(0);
		Network.Until([Counter, Frames]() { return ++(*Counter) >= Frames; });
	}

	static bool SlotHoldsItem(const URockInventory* Inventory, FRockInventorySlotHandle Slot)
	{
		return Inventory && Inventory->GetSlotByHandle(Slot).ItemHandle.IsValid();
	}

	static APlayerController* ServerController(FState& ServerState, int32 ClientIndex)
	{
		return ServerState.ClientConnections.IsValidIndex(ClientIndex) && ServerState.ClientConnections[ClientIndex]
			? ServerState.ClientConnections[ClientIndex]->PlayerController.Get() : nullptr;
	}

	/** The top inventory on this machine (its Owner is the actor), or null. */
	static URockInventory* FindChest(const ARockInventoryNetTestActor* Actor)
	{
		if (Actor)
		{
			for (URockInventory* Inventory : Actor->FindInventories())
			{
				if (Inventory->GetOwner() == Actor)
				{
					return Inventory;
				}
			}
		}
		return nullptr;
	}

	/** The inventory under the chest (its Owner is an inventory), or null. */
	static URockInventory* FindNested(const ARockInventoryNetTestActor* Actor)
	{
		if (Actor)
		{
			for (URockInventory* Inventory : Actor->FindInventories())
			{
				if (Cast<URockInventory>(Inventory->GetOwner()))
				{
					return Inventory;
				}
			}
		}
		return nullptr;
	}

	static URockItemDefinition* FindPlainDefinition()
	{
		TArray<URockItemDefinition*> All;
		URockItemRegistrySubsystem::GetInstance()->GetAllDefinitions(All);
		for (URockItemDefinition* Definition : All)
		{
			if (Definition && Definition->RuntimeInstanceClass.IsNull() && Definition->GridSize == FIntPoint(1, 1))
			{
				return Definition;
			}
		}
		return nullptr;
	}

	static void PutItemInStartSlot(URockInventory* Inventory, URockItemDefinition* Definition)
	{
		const FRockItemStackHandle Handle = Inventory->AddItemToInventory(FRockItemStack(Definition, 1));
		FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(FRockInventorySlotHandle{0});
		Entry.ItemHandle = Handle;
		Inventory->SetSlotByHandle(FRockInventorySlotHandle{0}, Entry);
	}

	TEST_METHOD(SecondClientSeesAContainerOnlyWhileItHasItOpen_AndANestedOneOnlyAfterOpeningIt)
	{
		Network.SpawnAndReplicate<ARockInventoryNetTestActor, &FState::Actor>();

		// A chest the owner client owns, built through the plugin's own registration: one item, one item instance, and an inventory under it.
		Network.ThenServer(TEXT("Create the chest, an instance and a nested inventory"), [this](FState& ServerState)
		{
			APlayerController* OwnerController = ServerController(ServerState, OwnerClient);
			APlayerController* ViewerController = ServerController(ServerState, ViewerClient);
			ASSERT_THAT(IsNotNull(OwnerController));
			ASSERT_THAT(IsNotNull(ViewerController));
			ARockInventoryNetTestActor* Actor = ServerState.Actor;
			ASSERT_THAT(IsNotNull(Actor));
			Actor->SetOwner(OwnerController);

			URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
			Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 4)};
			URockInventory* Chest = NewObject<URockInventory>(Actor);
			Chest->Owner = Actor;
			Actor->ServerInventory = Chest;
			Chest->Init(Config);

			URockItemDefinition* Definition = FindPlainDefinition();
			ASSERT_THAT(IsNotNull(Definition));
			// An item with a runtime instance, made through AddItemToInventory like in the game: the registry asset is given an instance class
			// for the call only (in memory; the clients live in this process and only read the definition by reference).
			Definition->RuntimeInstanceClass = URockItemInstance::StaticClass();
			PutItemInStartSlot(Chest, Definition);
			Definition->RuntimeInstanceClass = nullptr;
			URockItemInstance* Instance = Chest->GetItemBySlotHandle(StartSlot).GetRuntimeInstance();
			ASSERT_THAT(IsNotNull(Instance));
			Actor->ServerInstances.Add(Instance);

			URockInventory* Nested = NewObject<URockInventory>(Actor);
			Nested->Owner = Chest;
			Actor->ServerNested = Nested;
			Nested->Init(Config);
			PutItemInStartSlot(Nested, Definition);

			// The viewer's manager component carries the Observed list that gives the client its sync state
			URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(ViewerController);
			Manager->RegisterComponent();
			Manager->SetIsReplicated(true);

			// The chest is far from every player's pawn: the open must not depend on reach here
			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(Actor);
			ASSERT_THAT(IsNotNull(Access));
			FRockInventoryAccessPolicy Policy;
			Policy.Reach = ERockReachMode::Ignore;
			Access->SetPolicy(Chest, Policy);
			Access->SetPolicy(Nested, Policy);
			Actor->ForceNetUpdate();
		});

		// 1. The owner gets the chest, the instance and the nested inventory; the second client gets none of them.
		Network.UntilClient(TEXT("Owner receives the chest with its item, the instance and the nested inventory"), OwnerClient, [this](FState& ClientState)
		{
			return ClientState.Actor && SlotHoldsItem(FindChest(ClientState.Actor), StartSlot) && ClientState.Actor->FindInstances().Num() == 1
				&& SlotHoldsItem(FindNested(ClientState.Actor), StartSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Second client has nothing"), ViewerClient, [this](FState& ClientState)
		{
			ASSERT_THAT(IsNotNull(ClientState.Actor));
			ASSERT_THAT(AreEqual(0, ClientState.Actor->FindInventories().Num()));
			ASSERT_THAT(AreEqual(0, ClientState.Actor->FindInstances().Num()));
		});

		// 2. Opening the chest delivers it and its instance, not the nested inventory.
		Network.ThenServer(TEXT("Viewer opens the chest"), [this](FState& ServerState)
		{
			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(ServerState.Actor);
			ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(ServerController(ServerState, ViewerClient), ServerState.Actor->ServerInventory)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Second client receives the chest and its instance"), ViewerClient, [this](FState& ClientState)
		{
			return ClientState.Actor && SlotHoldsItem(FindChest(ClientState.Actor), StartSlot) && ClientState.Actor->FindInstances().Num() == 1;
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Second client has the chest but not the nested inventory, and it is Live"), ViewerClient, [this](FState& ClientState)
		{
			ASSERT_THAT(IsNull(FindNested(ClientState.Actor)));
			ASSERT_THAT(AreEqual(ERockInventorySyncState::Live, FindChest(ClientState.Actor)->GetSyncState()));
		});

		// 3. While open, moves follow.
		Network.ThenServer(TEXT("Move in the chest"), [this](FState& ServerState)
		{
			URockInventory* Chest = ServerState.Actor->ServerInventory;
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Chest, StartSlot, Chest, SecondSlot)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClients(TEXT("Both clients see the move"), [this](FState& ClientState)
		{
			return SlotHoldsItem(FindChest(ClientState.Actor), SecondSlot);
		});

		// 4. Opening the nested inventory delivers it.
		Network.ThenServer(TEXT("Viewer opens the nested inventory"), [this](FState& ServerState)
		{
			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(ServerState.Actor);
			ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(ServerController(ServerState, ViewerClient), ServerState.Actor->ServerNested)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Second client receives the nested inventory"), ViewerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(FindNested(ClientState.Actor), StartSlot);
		});

		// 5. Closing the chest stops its updates; the owner keeps getting them. The client's copy goes Stale.
		// (A change made in the same server step as the close can still reach the viewer: the group change takes effect at the next replication
		// update, which also carries that change. So the move comes a few frames later.)
		Network.ThenServer(TEXT("Viewer closes the chest"), [this](FState& ServerState)
		{
			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(ServerState.Actor);
			ASSERT_THAT(IsTrue(Access->Close(ServerController(ServerState, ViewerClient), ServerState.Actor->ServerInventory)));
			ServerState.Actor->ForceNetUpdate();
		});
		WaitFrames(SettleFrames);
		Network.ThenServer(TEXT("The item moves again"), [this](FState& ServerState)
		{
			URockInventory* Chest = ServerState.Actor->ServerInventory;
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Chest, SecondSlot, Chest, ThirdSlot)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Owner sees the second move"), OwnerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(FindChest(ClientState.Actor), ThirdSlot);
		});
		Network.UntilClient(TEXT("Second client's chest turns Stale"), ViewerClient, [this](FState& ClientState)
		{
			URockInventory* Chest = FindChest(ClientState.Actor);
			return Chest && Chest->GetSyncState() == ERockInventorySyncState::Stale;
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Second client's chest kept its old contents"), ViewerClient, [this](FState& ClientState)
		{
			URockInventory* Chest = FindChest(ClientState.Actor);
			ASSERT_THAT(IsNotNull(Chest));
			ASSERT_THAT(IsTrue(SlotHoldsItem(Chest, SecondSlot)));
			ASSERT_THAT(IsFalse(SlotHoldsItem(Chest, ThirdSlot)));
			// The nested inventory is still open and still live
			ASSERT_THAT(AreEqual(ERockInventorySyncState::Live, FindNested(ClientState.Actor)->GetSyncState()));
		});

		// 6. Walking away from the nested inventory closes it (the re-check) and its updates stop too.
		Network.ThenServer(TEXT("Viewer's reach to the nested inventory is lost"), [this](FState& ServerState)
		{
			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(ServerState.Actor);
			FRockInventoryAccessPolicy Policy;
			Policy.Reach = ERockReachMode::Custom;
			Policy.CustomReach = [](const AController&, const URockInventory&) { return false; };
			Access->SetPolicy(ServerState.Actor->ServerNested, Policy);
			Access->RecheckReach();
			ASSERT_THAT(IsFalse(Access->IsOpen(ServerController(ServerState, ViewerClient), ServerState.Actor->ServerNested)));
			ServerState.Actor->ForceNetUpdate();
		});
		WaitFrames(SettleFrames);
		Network.ThenServer(TEXT("Nested inventory changes"), [this](FState& ServerState)
		{
			URockInventory* Nested = ServerState.Actor->ServerNested;
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Nested, StartSlot, Nested, SecondSlot)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Owner sees the nested move"), OwnerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(FindNested(ClientState.Actor), SecondSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Second client's nested copy is Stale and unchanged"), ViewerClient, [this](FState& ClientState)
		{
			URockInventory* Nested = FindNested(ClientState.Actor);
			ASSERT_THAT(IsNotNull(Nested));
			ASSERT_THAT(AreEqual(ERockInventorySyncState::Stale, Nested->GetSyncState()));
			ASSERT_THAT(IsTrue(SlotHoldsItem(Nested, StartSlot)));
			ASSERT_THAT(IsFalse(SlotHoldsItem(Nested, SecondSlot)));
		});
	}
};

#endif // ENABLE_PIE_NETWORK_TEST
