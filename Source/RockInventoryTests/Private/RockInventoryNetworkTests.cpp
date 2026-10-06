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
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryTags.h"
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#include "Net/Subsystems/NetworkSubsystem.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogRockInventoryNetSpike, Log, All);

// T-68 spike: can a URockInventory subobject be gated per viewer with COND_NetGroup? One dedicated server, two clients.
// Client 0 owns the actor (NetGroupOwner). Client 1 is granted and revoked through a private net condition group.
// Findings and numbers are in Docs/RockInventory/DevNotes.md ("COND_NetGroup spike").
//
// These tests start PIE servers, which the project's default online setup (Redpoint EOS, Lyra sessions) cannot do headless, and the spike is about Iris.
// They are named Network.<Plugin>.<Area> so the default run (BRS., Skybrook.) does not match them. Run them with
// `RunTests.ps1 -Filter Network.RockInventory -Network`, which forces Iris and the Null online subsystem.
NETWORK_TEST_CLASS(RockInventoryNetGroupSpike, "Network.RockInventory.NetGroupSpike")
{
	struct FState : public FBasePIENetworkComponentState
	{
		ARockInventoryNetTestActor* Actor = nullptr;
	};

	FPIENetworkComponent<FState> Network{TestRunner, TestCommandBuilder, bInitializing};

	static constexpr int32 OwnerClient = 0;
	static constexpr int32 ViewerClient = 1;
	static constexpr int32 SettleFrames = 30;

	// Per-test scratch shared between latent steps. Plain values: the latent lambdas run later, on the same object.
	const FName ViewerGroup = TEXT("RockViewer_Spike1");
	FRockInventorySlotHandle StartSlot{0};
	FRockInventorySlotHandle SecondSlot{5};
	FRockInventorySlotHandle ThirdSlot{10};
	uint32 BytesIdle = 0;
	uint32 BytesOneMove = 0;
	uint32 BytesOwnerStart = 0;
	uint32 BytesViewerStart = 0;

	BEFORE_EACH()
	{
		// The project's online setup logs while the PIE servers start (Redpoint EOS has no dedicated server credentials here,
		// and the EOS net driver cannot set up P2P); CQTest skips the test body when errors are already logged. None of this is under test.
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

	static UNetConnection* ServerConnection(FState& ServerState, int32 ClientIndex)
	{
		return ServerState.ClientConnections.IsValidIndex(ClientIndex) ? ServerState.ClientConnections[ClientIndex] : nullptr;
	}

	static APlayerController* ServerController(FState& ServerState, int32 ClientIndex)
	{
		UNetConnection* Connection = ServerConnection(ServerState, ClientIndex);
		return Connection ? Connection->PlayerController.Get() : nullptr;
	}

	/** Registers the subobject on the actor with COND_NetGroup and puts it in the given groups. Replaces the COND_None registration made by URockInventory::Init. */
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

	/** A definition from the project's real item assets: assets are stable-named, so they replicate by reference (transient test definitions would arrive null). */
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

	TEST_METHOD(OwnerSeesInventory_UngrantedClientDoesNot_GrantAndRevokeBehave)
	{
		// 1. Replicate the actor to everyone. It is always relevant, so both clients get it.
		Network.SpawnAndReplicate<ARockInventoryNetTestActor, &FState::Actor>();

		Network.ThenServer(TEXT("Iris is in use"), [this](FState& ServerState)
		{
			ASSERT_THAT(IsNotNull(ServerState.World));
			ASSERT_THAT(IsNotNull(ServerState.World->GetNetDriver()));
			UE_LOG(LogRockInventoryNetSpike, Display, TEXT("Spike: net driver %s, Iris=%d"),
				*ServerState.World->GetNetDriver()->GetClass()->GetName(), ServerState.World->GetNetDriver()->IsUsingIrisReplication());
			ASSERT_THAT(IsTrue(ServerState.World->GetNetDriver()->IsUsingIrisReplication()));
		});

		// 2. Server: make client 0 the actor's owner, build the inventory and gate it to NetGroupOwner only.
		Network.ThenServer(TEXT("Create the gated inventory"), [this](FState& ServerState)
		{
			APlayerController* OwnerController = ServerController(ServerState, OwnerClient);
			ASSERT_THAT(IsNotNull(OwnerController));
			ARockInventoryNetTestActor* Actor = ServerState.Actor;
			ASSERT_THAT(IsNotNull(Actor));
			Actor->SetOwner(OwnerController);

			URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
			Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 4, 4)};

			URockInventory* Inventory = NewObject<URockInventory>(Actor);
			Inventory->Owner = Actor;
			Actor->ServerInventory = Inventory;
			Inventory->Init(Config); // registers with COND_None; replaced right below, before the next net update.
			GateSubObject(Actor, Inventory, {UE::Net::NetGroupOwner});

			URockItemDefinition* Definition = FindPlainDefinition();
			ASSERT_THAT(IsNotNull(Definition));
			const FRockItemStackHandle Handle = Inventory->AddItemToInventory(FRockItemStack(Definition, 1));
			FRockInventorySlotEntry Entry = Inventory->GetSlotByHandle(StartSlot);
			Entry.ItemHandle = Handle;
			Inventory->SetSlotByHandle(StartSlot, Entry);
			ASSERT_THAT(IsTrue(SlotHoldsItem(Inventory, StartSlot)));
		});

		// 3. The owner receives the inventory with its item; the other client, given the same time, receives nothing.
		Network.UntilClient(TEXT("Owner receives the inventory"), OwnerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, StartSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Ungranted client has no inventory"), ViewerClient, [this](FState& ClientState)
		{
			ASSERT_THAT(IsNotNull(ClientState.Actor));
			ASSERT_THAT(IsNull(ClientState.Actor->FindInventory()));
		});

		// 4. Grant client 1 through a private group: the player joins the group, the inventory joins the same group.
		Network.ThenServer(TEXT("Grant the viewer"), [this](FState& ServerState)
		{
			ARockInventoryNetTestActor* Actor = ServerState.Actor;
			APlayerController* ViewerController = ServerController(ServerState, ViewerClient);
			ASSERT_THAT(IsNotNull(ViewerController));
			UNetworkSubsystem* NetSubsystem = ServerState.World->GetSubsystem<UNetworkSubsystem>();
			NetSubsystem->GetNetConditionGroupManager().RegisterSubObjectInGroup(Actor->ServerInventory, ViewerGroup);
			UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(Actor->ServerInventory, ServerState.World);
			ViewerController->IncludeInNetConditionGroup(ViewerGroup);
			Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Granted client receives the inventory"), ViewerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, StartSlot);
		});

		// 5. Bytes: an idle window against a window with one move, per connection.
		Network.ThenServer(TEXT("Idle window start"), [this](FState& ServerState)
		{
			BytesOwnerStart = ServerConnection(ServerState, OwnerClient)->OutTotalBytes;
		});
		WaitFrames(SettleFrames);
		Network.ThenServer(TEXT("Idle window end, move once"), [this](FState& ServerState)
		{
			BytesIdle = ServerConnection(ServerState, OwnerClient)->OutTotalBytes - BytesOwnerStart;
			BytesOwnerStart = ServerConnection(ServerState, OwnerClient)->OutTotalBytes;
			BytesViewerStart = ServerConnection(ServerState, ViewerClient)->OutTotalBytes;
			URockInventory* Inventory = ServerState.Actor->ServerInventory;
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Inventory, StartSlot, Inventory, SecondSlot)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClients(TEXT("Both clients see the move"), [this](FState& ClientState)
		{
			URockInventory* Inventory = ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr;
			return SlotHoldsItem(Inventory, SecondSlot) && !SlotHoldsItem(Inventory, StartSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenServer(TEXT("Log bytes per move"), [this](FState& ServerState)
		{
			BytesOneMove = ServerConnection(ServerState, OwnerClient)->OutTotalBytes - BytesOwnerStart;
			const uint32 ViewerBytes = ServerConnection(ServerState, ViewerClient)->OutTotalBytes - BytesViewerStart;
			UE_LOG(LogRockInventoryNetSpike, Display,
				TEXT("Spike bytes: idle window (%d frames) %u B; window with one move: owner %u B, viewer %u B"),
				SettleFrames, BytesIdle, BytesOneMove, ViewerBytes);
		});

		// 6. Revoke client 1 and move again: the owner follows, the revoked client keeps a stale copy of the object.
		Network.ThenServer(TEXT("Revoke the viewer, move again"), [this](FState& ServerState)
		{
			APlayerController* ViewerController = ServerController(ServerState, ViewerClient);
			ASSERT_THAT(IsNotNull(ViewerController));
			ViewerController->RemoveFromNetConditionGroup(ViewerGroup);
			URockInventory* Inventory = ServerState.Actor->ServerInventory;
			ASSERT_THAT(IsTrue(URockInventoryLibrary::MoveItem(Inventory, SecondSlot, Inventory, ThirdSlot)));
			ServerState.Actor->ForceNetUpdate();
		});
		Network.UntilClient(TEXT("Owner sees the second move"), OwnerClient, [this](FState& ClientState)
		{
			return SlotHoldsItem(ClientState.Actor ? ClientState.Actor->FindInventory() : nullptr, ThirdSlot);
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Revoked client keeps a stale copy"), ViewerClient, [this](FState& ClientState)
		{
			URockInventory* Inventory = ClientState.Actor->FindInventory();
			// The subobject is not destroyed on the client, it just stops updating.
			ASSERT_THAT(IsNotNull(Inventory));
			ASSERT_THAT(IsTrue(SlotHoldsItem(Inventory, SecondSlot)));
			ASSERT_THAT(IsFalse(SlotHoldsItem(Inventory, ThirdSlot)));
		});
	}

	TEST_METHOD(ItemInstance_NeedsTheSameMembershipAsItsInventory)
	{
		Network.SpawnAndReplicate<ARockInventoryNetTestActor, &FState::Actor>();

		// An inventory gated to the owner, plus two instances: one registered through the plugin's own path (COND_None), one gated like the inventory.
		Network.ThenServer(TEXT("Create the inventory and two instances"), [this](FState& ServerState)
		{
			APlayerController* OwnerController = ServerController(ServerState, OwnerClient);
			ASSERT_THAT(IsNotNull(OwnerController));
			ARockInventoryNetTestActor* Actor = ServerState.Actor;
			Actor->SetOwner(OwnerController);

			URockInventoryConfig* Config = NewObject<URockInventoryConfig>(GetTransientPackage());
			Config->InventoryTabs = {FRockInventorySectionInfo(RockInventoryTags::Inventory_Section_Backpack, 0, 2, 2)};
			URockInventory* Inventory = NewObject<URockInventory>(Actor);
			Inventory->Owner = Actor;
			Actor->ServerInventory = Inventory;
			Inventory->Init(Config);
			GateSubObject(Actor, Inventory, {UE::Net::NetGroupOwner});

			// Through the plugin: SetOwningInventory registers the instance on the top-level owner with COND_None.
			URockItemInstance* Ungated = NewObject<URockItemInstance>(Inventory);
			Ungated->SetOwningInventory(Inventory);
			Actor->ServerInstances.Add(Ungated);

			URockItemInstance* Gated = NewObject<URockItemInstance>(Inventory);
			Gated->OwningInventory = Inventory;
			Actor->ServerInstances.Add(Gated);
			GateSubObject(Actor, Gated, {UE::Net::NetGroupOwner});
		});

		Network.UntilClient(TEXT("Owner receives both instances"), OwnerClient, [this](FState& ClientState)
		{
			return ClientState.Actor && ClientState.Actor->FindInstances().Num() == 2;
		});
		WaitFrames(SettleFrames);
		Network.ThenClient(TEXT("Viewer has no inventory; only the ungated instance leaks"), ViewerClient, [this](FState& ClientState)
		{
			ASSERT_THAT(IsNull(ClientState.Actor->FindInventory()));
			const int32 InstancesSeen = ClientState.Actor->FindInstances().Num();
			UE_LOG(LogRockInventoryNetSpike, Display, TEXT("Spike: ungranted client received %d of 2 instances (1 = only the COND_None one leaks)"), InstancesSeen);
			ASSERT_THAT(AreEqual(1, InstancesSeen));
		});
	}
};

#endif // ENABLE_PIE_NETWORK_TEST
