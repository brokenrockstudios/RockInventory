// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Replication/RockInventoryReplication.h"

#include "Access/RockInventoryAccessSubsystem.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/RockInventory.h"
#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#endif
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#include "Net/Subsystems/NetworkSubsystem.h"
#include "RockInventoryLogging.h"

static TAutoConsoleVariable<bool> CVarRockInventoryGatedReplication(
	TEXT("RockInventory.GatedReplication"), true,
	TEXT("1: inventories and their item instances replicate through COND_NetGroup (owner, open, proximity, shared). 0: to everyone (COND_None)."),
	ECVF_Default);

bool RockInventoryReplication::IsGatingEnabled()
{
	return CVarRockInventoryGatedReplication.GetValueOnAnyThread();
}

FName RockInventoryReplication::GetViewerGroup(const APlayerController& Controller)
{
	return FName(*FString::Printf(TEXT("RockViewer_%u"), Controller.GetUniqueID()));
}

void RockInventoryReplication::SetSubObjectGroups(const UObject* WorldContext, UObject* SubObject, TConstArrayView<FName> Groups)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UNetworkSubsystem* NetSubsystem = World ? World->GetSubsystem<UNetworkSubsystem>() : nullptr;
	if (!NetSubsystem || !SubObject)
	{
		return;
	}
	UE::Net::FNetConditionGroupManager& Manager = NetSubsystem->GetNetConditionGroupManager();

	TArray<FName> Current(Manager.GetSubObjectNetConditionGroups(FObjectKey(SubObject)));
	bool bChanged = false;
	for (const FName Group : Current)
	{
		if (!Groups.Contains(Group))
		{
			Manager.UnregisterSubObjectFromGroup(SubObject, Group);
			bChanged = true;
#if UE_WITH_IRIS
			// The engine only ever adds an object to an Iris group (UpdateSubObjectGroupMemberships), so a revoked viewer would keep receiving it.
			if (!UE::Net::IsSpecialNetConditionGroup(Group))
			{
				UE::Net::FReplicationSystemUtil::ForEachReplicationSystem(GEngine, World, [SubObject, Group](UReplicationSystem* ReplicationSystem)
				{
					if (ReplicationSystem->IsServer())
					{
						if (UObjectReplicationBridge* Bridge = ReplicationSystem->GetReplicationBridge())
						{
							const UE::Net::FNetRefHandle RefHandle = Bridge->GetReplicatedRefHandle(SubObject);
							if (RefHandle.IsValid())
							{
								ReplicationSystem->RemoveFromGroup(ReplicationSystem->GetOrCreateSubObjectFilter(Group), RefHandle);
							}
						}
					}
				});
			}
#endif
		}
	}
	for (const FName Group : Groups)
	{
		if (!Current.Contains(Group))
		{
			Manager.RegisterSubObjectInGroup(SubObject, Group);
			bChanged = true;
		}
	}
	if (bChanged)
	{
		UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(SubObject, World);
	}
}

bool RockInventoryReplication::RegisterSubObject(UObject* TopLevelOwner, UObject* SubObject, URockInventory* GatingInventory)
{
	// A nested inventory that follows its parent is gated like the root (and so are the instances in it)
	GatingInventory = GatingInventory ? GatingInventory->GetGatingRoot() : nullptr;
	const bool bGated = IsGatingEnabled();
	const ELifetimeCondition Condition = bGated ? COND_NetGroup : COND_None;

	// Groups first, then the registration: the object is never visible to a connection that is not in one of its groups.
	if (bGated)
	{
		TArray<FName> Groups;
		if (const URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(TopLevelOwner))
		{
			Groups = Access->GetReplicationGroups(GatingInventory);
		}
		else
		{
			Groups.Add(UE::Net::NetGroupOwner);
		}
		if (const UWorld* World = TopLevelOwner ? TopLevelOwner->GetWorld() : nullptr)
		{
			if (UNetworkSubsystem* NetSubsystem = World->GetSubsystem<UNetworkSubsystem>())
			{
				NetSubsystem->GetNetConditionGroupManager().RegisterSubObjectInMultipleGroups(SubObject, Groups);
			}
		}
	}

	if (UActorComponent* Component = Cast<UActorComponent>(TopLevelOwner))
	{
		Component->AddReplicatedSubObject(SubObject, Condition);
	}
	else if (AActor* Actor = Cast<AActor>(TopLevelOwner))
	{
		Actor->AddReplicatedSubObject(SubObject, Condition);
	}
	else
	{
		return false;
	}

	if (bGated)
	{
		// The replication system learns the memberships of an object that is already registered; one that just was needs the update too.
		if (const UWorld* World = TopLevelOwner->GetWorld())
		{
			UE::Net::FReplicationSystemUtil::UpdateSubObjectGroupMemberships(SubObject, World);
		}
	}
	if (bGated && GatingInventory)
	{
		if (URockInventoryAccessSubsystem* Access = URockInventoryAccessSubsystem::Get(TopLevelOwner))
		{
			Access->NotifyGatedObjectRegistered(*GatingInventory);
		}
	}
	return true;
}

void RockInventoryReplication::UnregisterSubObject(UObject* TopLevelOwner, UObject* SubObject)
{
	if (UActorComponent* Component = Cast<UActorComponent>(TopLevelOwner))
	{
		Component->RemoveReplicatedSubObject(SubObject);
	}
	else if (AActor* Actor = Cast<AActor>(TopLevelOwner))
	{
		Actor->RemoveReplicatedSubObject(SubObject);
	}
	else
	{
		return;
	}
	const UWorld* World = TopLevelOwner->GetWorld();
	if (UNetworkSubsystem* NetSubsystem = World ? World->GetSubsystem<UNetworkSubsystem>() : nullptr)
	{
		NetSubsystem->GetNetConditionGroupManager().UnregisterSubObjectFromAllGroups(SubObject);
	}
}
