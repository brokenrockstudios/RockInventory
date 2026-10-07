// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Access/RockInventoryAccessSubsystem.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/RockInventory.h"
#include "Net/Core/Misc/NetConditionGroupManager.h"
#include "Replication/RockInventoryReplication.h"
#include "RockInventoryLogging.h"
#include "Stats/Stats.h"

URockInventoryAccessSubsystem* URockInventoryAccessSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<URockInventoryAccessSubsystem>() : nullptr;
}

const AActor* URockInventoryAccessSubsystem::GetOwningActorOf(const URockInventory* Inventory)
{
	return Inventory ? const_cast<URockInventory*>(Inventory)->GetOwningActor() : nullptr;
}

bool URockInventoryAccessSubsystem::IsPersonal(const AController& Controller, const AActor& InventoryActor)
{
	return &InventoryActor == &Controller || &InventoryActor == Controller.GetPawn() || &InventoryActor == Controller.PlayerState;
}

const FRockInventoryAccessPolicy* URockInventoryAccessSubsystem::FindPolicy(const URockInventory* Inventory) const
{
	return Policies.Find(Inventory);
}

FRockInventoryAccessPolicy URockInventoryAccessSubsystem::GetPolicy(const URockInventory* Inventory) const
{
	const FRockInventoryAccessPolicy* Found = FindPolicy(Inventory);
	return Found ? *Found : FRockInventoryAccessPolicy();
}

void URockInventoryAccessSubsystem::SetPolicy(const URockInventory* Inventory, FRockInventoryAccessPolicy Policy)
{
	if (Inventory)
	{
		Policies.Add(Inventory, MoveTemp(Policy));
		RefreshReplication(*const_cast<URockInventory*>(Inventory));
	}
}

void URockInventoryAccessSubsystem::ClearPolicy(const URockInventory* Inventory)
{
	Policies.Remove(Inventory);
	if (Inventory)
	{
		RefreshReplication(*const_cast<URockInventory*>(Inventory));
	}
}

void URockInventoryAccessSubsystem::GrantShared(const URockInventory* Inventory, const AController* Controller, ERockInventoryRights Rights)
{
	if (!Inventory || !Controller)
	{
		return;
	}
	RevokeShared(Inventory, Controller);
	if (Rights != ERockInventoryRights::None)
	{
		SharedGrants.Add({Controller, Inventory, Rights});
	}
	RefreshReplication(*const_cast<URockInventory*>(Inventory));
}

void URockInventoryAccessSubsystem::RevokeShared(const URockInventory* Inventory, const AController* Controller)
{
	SharedGrants.RemoveAll([&](const FSharedGrant& Grant) { return Grant.Inventory == Inventory && Grant.Controller == Controller; });
	if (Inventory)
	{
		RefreshReplication(*const_cast<URockInventory*>(Inventory));
	}
}

bool URockInventoryAccessSubsystem::IsWithinReach(const AController& Controller, const URockInventory& Inventory, const AActor& InventoryActor, const FRockInventoryAccessPolicy& Policy) const
{
	switch (Policy.Reach)
	{
	case ERockReachMode::Ignore:
		return true;
	case ERockReachMode::Custom:
		return Policy.CustomReach && Policy.CustomReach(Controller, Inventory);
	case ERockReachMode::Default:
	default:
		{
			const APawn* Pawn = Controller.GetPawn();
			const float Reach = Policy.ReachOverride > 0.f ? Policy.ReachOverride : DefaultReach;
			return Pawn && Pawn->GetDistanceTo(&InventoryActor) <= Reach;
		}
	}
}

bool URockInventoryAccessSubsystem::IsOpenEntry(const AController* Controller, const URockInventory* Inventory) const
{
	return OpenEntries.ContainsByPredicate([&](const FOpenEntry& Entry) { return Entry.Controller == Controller && Entry.Inventory == Inventory; });
}

bool URockInventoryAccessSubsystem::IsOpen(const AController* Controller, const URockInventory* Inventory) const
{
	return Controller && Inventory && IsOpenEntry(Controller, Inventory);
}

FRockInventoryAccess URockInventoryAccessSubsystem::GetAccess(const AController* Controller, const URockInventory* Inventory) const
{
	FRockInventoryAccess Result;
	if (!Controller || !Inventory)
	{
		return Result;
	}
	// A nested inventory that follows its parent (weapon attachments) has its parent's access
	Inventory = Inventory->GetGatingRoot();
	const AActor* InventoryActor = GetOwningActorOf(Inventory);
	if (!InventoryActor)
	{
		return Result;
	}
	const FRockInventoryAccessPolicy Policy = GetPolicy(Inventory);
	// An owner-only nested inventory (secure container): the player it sits on, nobody else, whatever is opened or shared
	if (Inventory->NestedVisibility == ERockNestedVisibility::OwnerOnly)
	{
		if (IsPersonal(*Controller, *InventoryActor))
		{
			Result.Reasons = ERockAccessReason::Owner;
			Result.Rights = ERockInventoryRights::Full;
		}
		return Result;
	}
	auto Grant = [&Result](ERockAccessReason Reason, ERockInventoryRights Rights)
	{
		Result.Reasons |= Reason;
		Result.Rights = FMath::Max(Result.Rights, Rights);
	};

	if (IsPersonal(*Controller, *InventoryActor))
	{
		Grant(ERockAccessReason::Owner, ERockInventoryRights::Full);
	}
	if (IsOpenEntry(Controller, Inventory) && IsWithinReach(*Controller, *Inventory, *InventoryActor, Policy))
	{
		Grant(ERockAccessReason::Open, ERockInventoryRights::Full);
	}
	if (Policy.Owner.Get() == Controller)
	{
		const APawn* Pawn = Controller->GetPawn();
		const float Radius = Policy.ProximityRadius > 0.f ? Policy.ProximityRadius : DefaultProximityRadius;
		if (Pawn && Pawn->GetDistanceTo(InventoryActor) <= Radius)
		{
			Grant(ERockAccessReason::Proximity, ERockInventoryRights::View);
		}
	}
	for (const FSharedGrant& Shared : SharedGrants)
	{
		if (Shared.Controller == Controller && Shared.Inventory == Inventory)
		{
			Grant(ERockAccessReason::Shared, Shared.Rights);
		}
	}
	return Result;
}

bool URockInventoryAccessSubsystem::CanAccess(const AController* Controller, const URockInventory* Inventory, ERockInventoryRights Required) const
{
	return GetAccess(Controller, Inventory).Allows(Required);
}

ERockOpenResult URockInventoryAccessSubsystem::Open(const AController* Controller, const URockInventory* Inventory)
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return ERockOpenResult::NotAuthority;
	}
	Inventory = Inventory ? Inventory->GetGatingRoot() : nullptr;
	const AActor* InventoryActor = GetOwningActorOf(Inventory);
	if (!Controller || !InventoryActor)
	{
		return ERockOpenResult::InvalidArguments;
	}
	if (IsOpenEntry(Controller, Inventory))
	{
		return ERockOpenResult::AlreadyOpen;
	}

	const FRockInventoryAccessPolicy Policy = GetPolicy(Inventory);
	if (!IsPersonal(*Controller, *InventoryActor))
	{
		if (Inventory->NestedVisibility == ERockNestedVisibility::OwnerOnly)
		{
			return ERockOpenResult::NotOpenable;
		}
		bool bOpenable = false;
		switch (Policy.OpenableBy)
		{
		case ERockOpenableBy::Anyone:
			bOpenable = true;
			break;
		case ERockOpenableBy::OwnerOnly:
			bOpenable = Policy.Owner.Get() == Controller;
			break;
		case ERockOpenableBy::Auto:
		default:
			{
				// An inventory on a controller, a player state or a possessed pawn is closed to everyone else; a chest, a body or a dropped backpack is not.
				const APawn* AsPawn = Cast<APawn>(InventoryActor);
				const bool bOnAPlayer = InventoryActor->IsA<AController>() || InventoryActor->IsA<APlayerState>() || (AsPawn && (AsPawn->GetController() || AsPawn->GetPlayerState()));
				bOpenable = !bOnAPlayer;
			}
			break;
		}
		if (!bOpenable)
		{
			return ERockOpenResult::NotOpenable;
		}
		if (!IsWithinReach(*Controller, *Inventory, *InventoryActor, Policy))
		{
			return ERockOpenResult::OutOfReach;
		}
	}

	FRockInventoryOpenRequest Request;
	Request.Controller = Controller;
	Request.Inventory = Inventory;
	OnBeforeOpen.Broadcast(Request);
	if (Request.bVeto)
	{
		return ERockOpenResult::Vetoed;
	}

	OpenEntries.Add({Controller, Inventory});
	RefreshReplication(*const_cast<URockInventory*>(Inventory));
	OnAfterOpen.Broadcast(*Controller, *Inventory);
	return ERockOpenResult::Opened;
}

bool URockInventoryAccessSubsystem::Close(const AController* Controller, const URockInventory* Inventory, ERockCloseReason Reason)
{
	Inventory = Inventory ? Inventory->GetGatingRoot() : nullptr;
	const int32 Index = OpenEntries.IndexOfByPredicate([&](const FOpenEntry& Entry) { return Entry.Controller == Controller && Entry.Inventory == Inventory; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OpenEntries.RemoveAtSwap(Index);
	if (Inventory)
	{
		RefreshReplication(*const_cast<URockInventory*>(Inventory));
	}
	if (Controller && Inventory)
	{
		OnClosed.Broadcast(*Controller, *Inventory, Reason);
	}
	return true;
}

int32 URockInventoryAccessSubsystem::CloseAll(const AController* Controller)
{
	int32 Closed = 0;
	for (const URockInventory* Inventory : GetOpenInventories(Controller))
	{
		Closed += Close(Controller, Inventory) ? 1 : 0;
	}
	return Closed;
}

TArray<const URockInventory*> URockInventoryAccessSubsystem::GetOpenInventories(const AController* Controller) const
{
	TArray<const URockInventory*> Result;
	for (const FOpenEntry& Entry : OpenEntries)
	{
		if (Entry.Controller == Controller && Entry.Inventory.IsValid())
		{
			Result.Add(Entry.Inventory.Get());
		}
	}
	return Result;
}

TArray<const AController*> URockInventoryAccessSubsystem::GetViewers(const URockInventory* Inventory) const
{
	TArray<const AController*> Result;
	for (const FOpenEntry& Entry : OpenEntries)
	{
		if (Entry.Inventory == Inventory && Entry.Controller.IsValid())
		{
			Result.Add(Entry.Controller.Get());
		}
	}
	return Result;
}

void URockInventoryAccessSubsystem::RecheckReach()
{
	// Collect first: a handler of OnClosed may open or close containers.
	TArray<FOpenEntry> OutOfReach;
	for (int32 Index = OpenEntries.Num() - 1; Index >= 0; --Index)
	{
		const FOpenEntry Entry = OpenEntries[Index];
		const AController* Controller = Entry.Controller.Get();
		const URockInventory* Inventory = Entry.Inventory.Get();
		const AActor* InventoryActor = GetOwningActorOf(Inventory);
		if (!Controller || !Inventory || !InventoryActor)
		{
			OpenEntries.RemoveAtSwap(Index);
		}
		else if (!IsPersonal(*Controller, *InventoryActor) && !IsWithinReach(*Controller, *Inventory, *InventoryActor, GetPolicy(Inventory)))
		{
			OpenEntries.RemoveAtSwap(Index);
			OutOfReach.Add(Entry);
		}
	}
	SharedGrants.RemoveAll([](const FSharedGrant& Grant) { return !Grant.Controller.IsValid() || !Grant.Inventory.IsValid(); });
	for (auto It = Policies.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	RefreshAllReplication();
	for (const FOpenEntry& Entry : OutOfReach)
	{
		if (Entry.Controller.IsValid() && Entry.Inventory.IsValid())
		{
			OnClosed.Broadcast(*Entry.Controller.Get(), *Entry.Inventory.Get(), ERockCloseReason::OutOfReach);
		}
	}
}

TArray<APlayerController*> URockInventoryAccessSubsystem::GetViewerControllers(const URockInventory* Inventory) const
{
	TArray<APlayerController*> Result;
	if (!Inventory)
	{
		return Result;
	}
	Inventory = Inventory->GetGatingRoot();
	TArray<const AController*> Candidates;
	for (const FOpenEntry& Entry : OpenEntries)
	{
		if (Entry.Inventory == Inventory && Entry.Controller.IsValid())
		{
			Candidates.AddUnique(Entry.Controller.Get());
		}
	}
	for (const FSharedGrant& Shared : SharedGrants)
	{
		if (Shared.Inventory == Inventory && Shared.Controller.IsValid())
		{
			Candidates.AddUnique(Shared.Controller.Get());
		}
	}
	if (const FRockInventoryAccessPolicy* Policy = FindPolicy(Inventory))
	{
		if (const AController* PolicyOwner = Policy->Owner.Get())
		{
			Candidates.AddUnique(PolicyOwner);
		}
	}
	for (const AController* Candidate : Candidates)
	{
		// Only a player controller has a connection to replicate to
		APlayerController* Player = const_cast<APlayerController*>(Cast<APlayerController>(Candidate));
		if (Player && GetAccess(Player, Inventory).Allows(ERockInventoryRights::View))
		{
			Result.Add(Player);
		}
	}
	return Result;
}

TArray<FName> URockInventoryAccessSubsystem::GetReplicationGroups(const URockInventory* Inventory) const
{
	TArray<FName> Groups;
	Groups.Add(UE::Net::NetGroupOwner);
	for (const APlayerController* Viewer : GetViewerControllers(Inventory))
	{
		Groups.AddUnique(RockInventoryReplication::GetViewerGroup(*Viewer));
	}
	return Groups;
}

void URockInventoryAccessSubsystem::RefreshReplication(URockInventory& Inventory)
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || !RockInventoryReplication::IsGatingEnabled())
	{
		return;
	}
	// The root's viewers cover the nested inventories that follow it; Viewed and Observed are keyed by the root
	URockInventory* RootInventory = Inventory.GetGatingRoot();
	if (RootInventory != &Inventory)
	{
		RefreshReplication(*RootInventory);
		return;
	}

	const TArray<APlayerController*> NowViewing = GetViewerControllers(&Inventory);
	TArray<TWeakObjectPtr<APlayerController>> Before;
	if (const TArray<TWeakObjectPtr<APlayerController>>* Found = Viewed.Find(&Inventory))
	{
		Before = *Found;
	}

	// The objects join the groups first, then the controllers (the order the T-68 spike verified).
	TArray<FName> Groups;
	Groups.Add(UE::Net::NetGroupOwner);
	for (APlayerController* Viewer : NowViewing)
	{
		Groups.AddUnique(RockInventoryReplication::GetViewerGroup(*Viewer));
	}
	Inventory.ForEachGatedObject([&](UObject* Object) { RockInventoryReplication::SetSubObjectGroups(this, Object, Groups); });
	for (APlayerController* Viewer : NowViewing)
	{
		Viewer->IncludeInNetConditionGroup(RockInventoryReplication::GetViewerGroup(*Viewer));
	}

	if (NowViewing.IsEmpty())
	{
		Viewed.Remove(&Inventory);
	}
	else
	{
		TArray<TWeakObjectPtr<APlayerController>>& Tracked = Viewed.FindOrAdd(&Inventory);
		Tracked.Reset();
		for (APlayerController* Viewer : NowViewing)
		{
			Tracked.Add(Viewer);
		}
	}

	// Tell the controllers whose view changed, so their client can show Syncing, Live or Stale.
	for (APlayerController* Viewer : NowViewing)
	{
		if (!Before.Contains(Viewer))
		{
			if (URockInventoryManagerComponent* Manager = URockInventoryManagerComponent::FindFor(Viewer))
			{
				Manager->ServerSetObserved(&Inventory, true);
			}
		}
	}
	for (const TWeakObjectPtr<APlayerController>& Old : Before)
	{
		if (Old.IsValid() && !NowViewing.Contains(Old.Get()))
		{
			if (URockInventoryManagerComponent* Manager = URockInventoryManagerComponent::FindFor(Old.Get()))
			{
				Manager->ServerSetObserved(&Inventory, false);
			}
		}
	}
}

void URockInventoryAccessSubsystem::NotifyGatedObjectRegistered(URockInventory& Inventory)
{
	// A new item instance joined the groups of its inventory at registration; only an inventory that is not tracked yet needs a look.
	if (!Viewed.Contains(&Inventory) && !GetViewerControllers(&Inventory).IsEmpty())
	{
		RefreshReplication(Inventory);
	}
}

void URockInventoryAccessSubsystem::RefreshAllReplication()
{
	TArray<TWeakObjectPtr<URockInventory>> Candidates;
	auto AddCandidate = [&Candidates](const URockInventory* Inventory)
	{
		if (Inventory)
		{
			Candidates.AddUnique(const_cast<URockInventory*>(Inventory));
		}
	};
	for (const FOpenEntry& Entry : OpenEntries)
	{
		AddCandidate(Entry.Inventory.Get());
	}
	for (const FSharedGrant& Shared : SharedGrants)
	{
		AddCandidate(Shared.Inventory.Get());
	}
	for (const TPair<TWeakObjectPtr<const URockInventory>, FRockInventoryAccessPolicy>& Pair : Policies)
	{
		if (Pair.Value.Owner.IsValid())
		{
			AddCandidate(Pair.Key.Get());
		}
	}
	for (auto It = Viewed.CreateIterator(); It; ++It)
	{
		if (It.Key().IsValid())
		{
			AddCandidate(It.Key().Get());
		}
		else
		{
			It.RemoveCurrent();
		}
	}
	for (const TWeakObjectPtr<URockInventory>& Candidate : Candidates)
	{
		if (URockInventory* Inventory = Candidate.Get())
		{
			// RefreshReplication changes Viewed and calls into managers, so the candidate list is a copy
			const TArray<APlayerController*> Now = GetViewerControllers(Inventory);
			const TArray<TWeakObjectPtr<APlayerController>>* Old = Viewed.Find(Inventory);
			bool bSame = (Old ? Old->Num() : 0) == Now.Num();
			for (int32 Index = 0; bSame && Old && Index < Now.Num(); ++Index)
			{
				bSame = Old->Contains(Now[Index]);
			}
			if (!bSame)
			{
				RefreshReplication(*Inventory);
			}
		}
	}
}

void URockInventoryAccessSubsystem::Tick(float DeltaTime)
{
	if (OpenEntries.IsEmpty() && Policies.IsEmpty() && SharedGrants.IsEmpty() && Viewed.IsEmpty())
	{
		SinceRecheck = 0.f;
		return;
	}
	SinceRecheck += DeltaTime;
	if (SinceRecheck >= RecheckInterval)
	{
		SinceRecheck = 0.f;
		RecheckReach();
	}
}

TStatId URockInventoryAccessSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URockInventoryAccessSubsystem, STATGROUP_Tickables);
}
