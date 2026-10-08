// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryNetTestActor.h"

#include "Components/PIENetworkComponent.h"

#if ENABLE_PIE_NETWORK_TEST

#include "Access/RockInventoryAccessSubsystem.h"
#include "Client/RockInventoryClientModel.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryConfig.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Item/RockItemDefinition.h"
#include "Misc/RockInventoryTags.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogRockInventoryNetPrediction, Log, All);

// T-78: prediction over a real connection with added latency. A dedicated server and one client (Iris); the client's manager component
// moves an item in a chest it owns. The move has to show on the client's model at once, while the replicated chest still holds the item
// where it was (the command is still on its way), and when the server's answer and the replicated state arrive the model must not
// announce anything more: the server agreed, so there is no visible correction. Same harness and run command as
// RockInventoryNetworkTests.cpp (T-68): `RunTests.ps1 -Filter Network.RockInventory -Network`. Findings: Docs/RockInventory/DevNotes.md ("Prediction (T-78)").
NETWORK_TEST_CLASS(RockInventoryNetPrediction, "Network.RockInventory.Prediction")
{
	struct FState : public FBasePIENetworkComponentState
	{
		ARockInventoryNetTestActor* Actor = nullptr;
	};

	FPIENetworkComponent<FState> Network{TestRunner, TestCommandBuilder, bInitializing};

	static constexpr int32 OwnerClient = 0;
	static constexpr int32 SettleFrames = 30;
	/** Milliseconds the client's outgoing packets are held back, so the prediction has to carry the item across several frames. */
	static constexpr int32 LatencyMs = 150;

	FRockInventorySlotHandle StartSlot{0};
	FRockInventorySlotHandle SecondSlot{5};

	URockInventoryClientModel* Model = nullptr;
	int32 Announced = 0;
	bool bShownAtOnce = false;
	bool bReplicatedStillOld = false;

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
			.WithClients(1)
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

	static URockInventory* FindChest(const ARockInventoryNetTestActor* Actor)
	{
		TArray<URockInventory*> Inventories = Actor ? Actor->FindInventories() : TArray<URockInventory*>();
		return Inventories.IsEmpty() ? nullptr : Inventories[0];
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

	TEST_METHOD(AMoveShowsAtOnce_AndTheServerAgreeingIsNotAVisibleCorrection)
	{
		Network.SpawnAndReplicate<ARockInventoryNetTestActor, &FState::Actor>();

		Network.ThenServer(TEXT("Create a chest the owner has open, with one item"), [this](FState& ServerState)
		{
			APlayerController* OwnerController = ServerController(ServerState, OwnerClient);
			ASSERT_THAT(IsNotNull(OwnerController));
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
			const FRockItemStackHandle Handle = Chest->AddItemToInventory(FRockItemStack(Definition, 1));
			FRockInventorySlotEntry Entry = Chest->GetSlotByHandle(StartSlot);
			Entry.ItemHandle = Handle;
			Chest->SetSlotByHandle(StartSlot, Entry);

			// The player's manager component: the client sends its commands through it
			URockInventoryManagerComponent* Manager = NewObject<URockInventoryManagerComponent>(OwnerController);
			Manager->RegisterComponent();
			Manager->SetIsReplicated(true);

			URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(Actor);
			ASSERT_THAT(IsNotNull(Access));
			FRockInventoryAccessPolicy Policy;
			Policy.Reach = ERockReachMode::Ignore;
			Access->SetPolicy(Chest, Policy);
			ASSERT_THAT(AreEqual(ERockOpenResult::Opened, Access->Open(OwnerController, Chest)));
			Actor->ForceNetUpdate();
		});

		Network.UntilClient(TEXT("Owner receives the chest, its item and the manager component"), OwnerClient, [this](FState& ClientState)
		{
			return ClientState.Actor && SlotHoldsItem(FindChest(ClientState.Actor), StartSlot)
				&& URockInventoryManagerComponent::FindLocal(ClientState.Actor->GetWorld()) != nullptr;
		});
		WaitFrames(SettleFrames);

		Network.ThenClient(TEXT("Owner moves the item with the client's packets delayed"), OwnerClient, [this](FState& ClientState)
		{
			URockInventory* Chest = FindChest(ClientState.Actor);
			URockInventoryManagerComponent* Manager = URockInventoryManagerComponent::FindLocal(ClientState.Actor->GetWorld());
			ASSERT_THAT(IsNotNull(Chest));
			ASSERT_THAT(IsNotNull(Manager));
			Model = URockInventoryClientModelSubsystem::GetModel(Chest);
			ASSERT_THAT(IsNotNull(Model));
			Model->OnChanged.AddLambda([this](URockInventoryClientModel&, const FRockInventoryPresentationDiff&) { ++Announced; });
			Announced = 0;

			GEngine->Exec(ClientState.Actor->GetWorld(), *FString::Printf(TEXT("Net PktLag=%d"), LatencyMs));

			ASSERT_THAT(IsTrue(Manager->MoveItem(FRockMoveItemTransaction(Manager->GetOwningController(), Chest, StartSlot, Chest, SecondSlot))));
			bShownAtOnce = Model->GetSlotByHandle(SecondSlot).ItemHandle.IsValid()
				&& !Model->GetSlotByHandle(StartSlot).ItemHandle.IsValid();
			bReplicatedStillOld = SlotHoldsItem(Chest, StartSlot) && !SlotHoldsItem(Chest, SecondSlot);
			UE_LOG(LogRockInventoryNetPrediction, Display, TEXT("Prediction: shown at once %d, replicated chest still old %d, announced %d, pending %d"),
				bShownAtOnce, bReplicatedStillOld, Announced, Manager->GetNumPendingCommands());
			ASSERT_THAT(IsTrue(bShownAtOnce));
			ASSERT_THAT(IsTrue(bReplicatedStillOld));
			ASSERT_THAT(AreEqual(1, Announced));
		});

		Network.UntilClient(TEXT("The move replicates back and the prediction settles"), OwnerClient, [this](FState& ClientState)
		{
			URockInventoryManagerComponent* Manager = URockInventoryManagerComponent::FindLocal(ClientState.Actor->GetWorld());
			return SlotHoldsItem(FindChest(ClientState.Actor), SecondSlot) && Manager && Manager->GetNumPendingCommands() == 0;
		});
		WaitFrames(SettleFrames);

		Network.ThenClient(TEXT("The model never changed again"), OwnerClient, [this](FState& ClientState)
		{
			GEngine->Exec(ClientState.Actor->GetWorld(), TEXT("Net PktLag=0"));
			UE_LOG(LogRockInventoryNetPrediction, Display, TEXT("Prediction: announced after the round trip %d"), Announced);
			ASSERT_THAT(AreEqual(1, Announced));
			ASSERT_THAT(IsTrue(Model->GetSlotByHandle(SecondSlot).ItemHandle.IsValid()));
			ASSERT_THAT(IsFalse(Model->GetSlotByHandle(StartSlot).ItemHandle.IsValid()));
		});
		Network.ThenServer(TEXT("The server has the item in the second slot"), [this](FState& ServerState)
		{
			ASSERT_THAT(IsTrue(SlotHoldsItem(ServerState.Actor->ServerInventory, SecondSlot)));
			ASSERT_THAT(IsFalse(SlotHoldsItem(ServerState.Actor->ServerInventory, StartSlot)));
		});
	}
};

#endif // ENABLE_PIE_NETWORK_TEST
