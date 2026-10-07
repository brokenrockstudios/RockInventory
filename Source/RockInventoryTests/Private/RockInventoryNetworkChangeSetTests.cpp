// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryNetTestActor.h"

#include "Components/PIENetworkComponent.h"

#if ENABLE_PIE_NETWORK_TEST

#include "Engine/NetConnection.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryConfig.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryTags.h"
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#include "Net/Subsystems/NetworkSubsystem.h"
#include "RockInventoryTestListener.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogRockInventoryNetChangeSet, Log, All);

// T-127: the client change-set flush over a real connection. T-74 flushes the client's batch from URockInventory::PostNetReceive; its
// other tests call the array callbacks and PostNetReceive by hand. Here a dedicated server and two clients (Iris) replicate a gated
// inventory, and each client listens to OnChangeBatch on its own copy. Same setup, same reasons and same run command as
// RockInventoryNetworkTests.cpp (T-68): `RunTests.ps1 -Filter Network.RockInventory -Network`. Findings are in Docs/RockInventory/DevNotes.md.
NETWORK_TEST_CLASS(RockInventoryNetChangeSet, "Network.RockInventory.ChangeSet")
{
	struct FState : public FBasePIENetworkComponentState
	{
		ARockInventoryNetTestActor* Actor = nullptr;
	};

	FPIENetworkComponent<FState> Network{TestRunner, TestCommandBuilder, bInitializing};

	static constexpr int32 OwnerClient = 0;
	static constexpr int32 ViewerClient = 1;
	static constexpr int32 SettleFrames = 30;

	const FName ViewerGroup = TEXT("RockViewer_ChangeSet");
	FRockInventorySlotHandle StartSlot{0};
	FRockInventorySlotHandle SecondSlot{5};
	FRockInventorySlotHandle ThirdSlot{10};
	FRockItemStackHandle ItemHandle;
	int32 RevisionBeforeStep = 0;
	int32 RevisionAfterStep = 0;

	// One listener per client, bound to that client's copy of the inventory. Strong pointers: the latent steps run across frames.
	TStrongObjectPtr<URockInventoryTestListener> Listeners[2];

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

	static void GateSubObject(ARockInventoryNetTestActor* Actor, UObject* SubObject, const TArray<FName>& Groups)
	{
		Actor->RemoveReplicatedSubObject(SubObject);
		UNetworkSubsystem* NetSubsystem = Actor->GetWorld()->GetSubsystem<UNetworkSubsystem>();
		for (const FName Group : Groups)
		{
			NetSubsystem->GetNetConditionGroupManager().RegisterSubObjectInGroup(SubObject, Group);
		}
		Actor->AddReplicatedSubObject(SubObject, COND_NetGroup);
		UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(SubObject, Actor->GetWorld());
		Actor->ForceNetUpdate();
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

	/** Starts listening to this client's copy of the inventory, with empty records. */
	void BindListener(int32 ClientIndex, FState& ClientState)
	{
		URockInventory* Inventory = ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr;
		ASSERT_THAT(IsNotNull(Inventory));
		Listeners[ClientIndex].Reset(NewObject<URockInventoryTestListener>(GetTransientPackage()));
		Inventory->OnChangeBatch.AddDynamic(Listeners[ClientIndex].Get(), &URockInventoryTestListener::OnChangeBatch);
		Inventory->OnSlotChanged.AddDynamic(Listeners[ClientIndex].Get(), &URockInventoryTestListener::OnSlotChanged);
		Inventory->OnItemChanged.AddDynamic(Listeners[ClientIndex].Get(), &URockInventoryTestListener::OnItemChanged);
	}

	/**
	 * What one replication update must have produced on this client: exactly one batch carrying the replicated Revision (the server's value
	 * after the step, which rose by ExpectedRevisionSteps operations), the legacy delegates replayed from that batch, and the item-to-slot
	 * lookup agreeing with the slots.
	 */
	void CheckClientFlush(int32 ClientIndex, FState& ClientState, int32 ExpectedRevisionSteps, FRockInventorySlotHandle ExpectedSlot)
	{
		URockInventory* Inventory = ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr;
		ASSERT_THAT(IsNotNull(Inventory));
		const URockInventoryTestListener* Listener = Listeners[ClientIndex].Get();
		ASSERT_THAT(IsNotNull(Listener));

		UE_LOG(LogRockInventoryNetChangeSet, Display, TEXT("ChangeSet client %d: server revision %d -> %d, client revision %u, batches %d, call order \"%s\", slot deltas %d, item deltas %d"),
			ClientIndex, RevisionBeforeStep, RevisionAfterStep, Inventory->GetRevision(), Listener->Batches.Num(), *Listener->CallOrder,
			Listener->SlotDeltas.Num(), Listener->ItemDeltas.Num());
		ASSERT_THAT(AreEqual(ExpectedRevisionSteps, RevisionAfterStep - RevisionBeforeStep));
		ASSERT_THAT(AreEqual(1, Listener->Batches.Num()));
		const FRockInventoryChangeBatch& Batch = Listener->Batches[0];
		ASSERT_THAT(IsTrue(Batch.Inventory == Inventory));
		ASSERT_THAT(AreEqual(RevisionAfterStep, Batch.Revision));
		ASSERT_THAT(AreEqual(RevisionAfterStep, static_cast<int32>(Inventory->GetRevision())));
		ASSERT_THAT(IsFalse(Batch.IsEmpty()));

		// The legacy per-delta delegates are replayed from the batch, after it, once.
		ASSERT_THAT(IsTrue(Listener->CallOrder.StartsWith(TEXT("B"))));
		ASSERT_THAT(AreEqual(Batch.SlotDeltas.Num(), Listener->SlotDeltas.Num()));
		ASSERT_THAT(AreEqual(Batch.ItemDeltas.Num(), Listener->ItemDeltas.Num()));

		// The reverse index, kept by the array callbacks, agrees with the slots after the update.
		ASSERT_THAT(IsTrue(SlotHoldsItem(Inventory, ExpectedSlot)));
		const FRockInventorySlotEntry* Found = Inventory->GetSlotByItemHandlePtr(ItemHandle);
		ASSERT_THAT(IsNotNull(Found));
		ASSERT_THAT(IsTrue(Found->SlotHandle == ExpectedSlot));
	}

	void ResetListeners()
	{
		for (TStrongObjectPtr<URockInventoryTestListener>& Listener : Listeners)
		{
			if (Listener.IsValid())
			{
				Listener->Batches.Reset();
				Listener->SlotDeltas.Reset();
				Listener->ItemDeltas.Reset();
				Listener->CallOrder.Reset();
			}
		}
	}

	TEST_METHOD(ClientFlushesOneBatchPerReplicationUpdate_WithReplicatedRevision)
	{
		Network.SpawnAndReplicate<ARockInventoryNetTestActor, &FState::Actor>();

		// An inventory gated to its owner and to a granted viewer, one item in the start slot.
		Network.ThenServer(TEXT("Create the gated inventory and grant the viewer"), [this](FState& ServerState)
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
			URockInventory* Inventory = NewObject<URockInventory>(Actor);
			Inventory->Owner = Actor;
			Actor->ServerInventory = Inventory;
			Inventory->Init(Config);
			GateSubObject(Actor, Inventory, {UE::Net::NetGroupOwner, ViewerGroup});
			ViewerController->IncludeInNetConditionGroup(ViewerGroup);

			URockItemDefinition* Definition = FindPlainDefinition();
			ASSERT_THAT(IsNotNull(Definition));
			ItemHandle = Inventory->AddItemToInventory(FRockItemStack(Definition, 1));
			FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(StartSlot);
			Entry.ItemHandle = ItemHandle;
			Inventory->SetSlotByHandle(StartSlot, Entry);
			ASSERT_THAT(IsTrue(SlotHoldsItem(Inventory, StartSlot)));
		});
		Network.UntilClients(TEXT("Both clients receive the inventory with its item"), [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, StartSlot);
		});
		WaitFrames(SettleFrames);

		// Listen from here on; what arrived so far (the full state) is not under test.
		Network.ThenClient(TEXT("Owner listens"), OwnerClient, [this](FState& ClientState) { BindListener(OwnerClient, ClientState); });
		Network.ThenClient(TEXT("Viewer listens"), ViewerClient, [this](FState& ClientState) { BindListener(ViewerClient, ClientState); });

		// 1. One move is one server operation: one revision step, one batch on each client.
		Network.ThenServer(TEXT("Move once"), [this](FState& ServerState)
		{
			URockInventory* Inventory = ServerState.Actor->ServerInventory;
			RevisionBeforeStep = static_cast<int32>(Inventory->GetRevision());
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Inventory, StartSlot, Inventory, SecondSlot)));
			RevisionAfterStep = static_cast<int32>(Inventory->GetRevision());
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClients(TEXT("Both clients see the move"), [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, SecondSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Owner: one batch, replicated revision, index follows"), OwnerClient,
			[this](FState& ClientState) { CheckClientFlush(OwnerClient, ClientState, 1, SecondSlot); });
		Network.ThenClient(TEXT("Viewer: one batch, replicated revision, index follows"), ViewerClient,
			[this](FState& ClientState) { CheckClientFlush(ViewerClient, ClientState, 1, SecondSlot); });

		// 2. Two operations inside one server step reach the clients in one update: still one batch, and the revision has risen by two.
		Network.ThenClient(TEXT("Clear both clients' records"), OwnerClient, [this](FState&) { ResetListeners(); });
		Network.ThenServer(TEXT("Move twice in one step"), [this](FState& ServerState)
		{
			URockInventory* Inventory = ServerState.Actor->ServerInventory;
			RevisionBeforeStep = static_cast<int32>(Inventory->GetRevision());
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Inventory, SecondSlot, Inventory, ThirdSlot)));
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Inventory, ThirdSlot, Inventory, StartSlot)));
			RevisionAfterStep = static_cast<int32>(Inventory->GetRevision());
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClients(TEXT("Both clients see the item back in the start slot"), [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, StartSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Owner: one batch for two operations"), OwnerClient,
			[this](FState& ClientState) { CheckClientFlush(OwnerClient, ClientState, 2, StartSlot); });
		Network.ThenClient(TEXT("Viewer: one batch for two operations"), ViewerClient,
			[this](FState& ClientState) { CheckClientFlush(ViewerClient, ClientState, 2, StartSlot); });
	}
};

#endif // ENABLE_PIE_NETWORK_TEST
