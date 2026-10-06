// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "RockInventoryAccessSubsystem.generated.h"

class AActor;
class AController;
class URockInventory;

/** What a player may do with a container. Ordered: a higher value includes the lower ones. Gameplay can add a rule by answering with less than Full. */
enum class ERockInventoryRights : uint8
{
	None,
	/** See the contents (proximity prefetch of your own base containers). */
	View,
	/** Take some items (reserved for a pickpocket style rule; nothing grants it yet). */
	LimitedTake,
	/** Move items in and out. What every server command asks for today. */
	Full,
};

/** Why a player has access. A bit set: several reasons can hold at once. */
enum class ERockAccessReason : uint8
{
	None = 0,
	/** The inventory sits on the player's own controller, pawn or player state. Always, at any distance. */
	Owner = 1 << 0,
	/** The player opened it explicitly and is still within reach. */
	Open = 1 << 1,
	/** A container the player owns (see FRockInventoryAccessPolicy::Owner) is near: see only, until opened. */
	Proximity = 1 << 2,
	/** A per-container grant (GrantShared). A stub: nothing in gameplay hands these out yet. */
	Shared = 1 << 3,
};
ENUM_CLASS_FLAGS(ERockAccessReason);

/** The answer to "what may this player do with this inventory": the rights and the reasons behind them. */
struct ROCKINVENTORYRUNTIME_API FRockInventoryAccess
{
	ERockInventoryRights Rights = ERockInventoryRights::None;
	ERockAccessReason Reasons = ERockAccessReason::None;

	bool Allows(ERockInventoryRights Required) const { return Required != ERockInventoryRights::None && Rights >= Required; }
};

/** Who may open a container. */
enum class ERockOpenableBy : uint8
{
	/** Anyone in reach, unless the inventory sits on a controller, a player state or a pawn that has a controller or player state (then nobody but its player). Chests, bodies and dropped backpacks are open to all. */
	Auto,
	Anyone,
	/** Only the player it sits on and the policy Owner. */
	OwnerOnly,
};

/** How the reach rule is evaluated for opening and for keeping a container open. */
enum class ERockReachMode : uint8
{
	/** The player's pawn is within ReachOverride (or the subsystem's DefaultReach) of the inventory's owning actor. */
	Default,
	/** No distance rule: the container stays open until closed (a remote terminal). */
	Ignore,
	/** CustomReach decides. */
	Custom,
};

enum class ERockOpenResult : uint8
{
	Opened,
	AlreadyOpen,
	NotAuthority,
	InvalidArguments,
	NotOpenable,
	OutOfReach,
	Vetoed,
};

enum class ERockCloseReason : uint8
{
	/** Close or CloseAll was called. */
	Requested,
	/** The periodic re-check found the player out of reach. */
	OutOfReach,
};

/** Per-container rules, registered with SetPolicy. A container without one uses the defaults below. */
struct ROCKINVENTORYRUNTIME_API FRockInventoryAccessPolicy
{
	ERockOpenableBy OpenableBy = ERockOpenableBy::Auto;
	ERockReachMode Reach = ERockReachMode::Default;
	/** Default reach mode only: > 0 replaces the subsystem's DefaultReach (cm). */
	float ReachOverride = 0.f;
	/** Custom reach mode only. Called for the open and for every re-check. */
	TFunction<bool(const AController&, const URockInventory&)> CustomReach;
	/** The player this container belongs to (their base chest): gets View access nearby, and may open an OwnerOnly container. Not the same as sitting on their pawn. */
	TWeakObjectPtr<const AController> Owner;
	/** > 0 replaces the subsystem's DefaultProximityRadius (cm) for the Proximity reason. */
	float ProximityRadius = 0.f;
};

/** Raised before an open is accepted; a handler sets bVeto (for example a homebase checking an approved player list). */
struct FRockInventoryOpenRequest
{
	const AController* Controller = nullptr;
	const URockInventory* Inventory = nullptr;
	bool bVeto = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FRockInventoryBeforeOpen, FRockInventoryOpenRequest&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FRockInventoryAfterOpen, const AController&, const URockInventory&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FRockInventoryClosed, const AController&, const URockInventory&, ERockCloseReason);

/**
 * Server-side registry of who may touch which inventory (T-75). It answers CanAccess for every server command and
 * holds the explicit opens. Deny by default: a player has access only through one of the reasons in ERockAccessReason.
 *
 * Authority only: the opens live on the server. A player may have several containers open and several players may
 * have the same one open. An open ends on Close, or when the periodic check (RecheckReach, every RecheckInterval)
 * finds the player out of reach. Replication is not gated by this yet (T-76).
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryAccessSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	static URockInventoryAccessSubsystem* Get(const UObject* WorldContext);

	/** Default reach (cm) from the player's pawn to the inventory's owning actor. */
	float DefaultReach = 500.f;
	/** Default radius (cm) of the Proximity reason. */
	float DefaultProximityRadius = 2000.f;
	/** Seconds between reach re-checks of the open containers. */
	float RecheckInterval = 0.25f;

	/** The rights and reasons the controller has for the inventory right now. Nothing is cached: reach is evaluated at the call. */
	FRockInventoryAccess GetAccess(const AController* Controller, const URockInventory* Inventory) const;

	/** True when the controller's rights reach Required. */
	bool CanAccess(const AController* Controller, const URockInventory* Inventory, ERockInventoryRights Required = ERockInventoryRights::Full) const;

	/** Opens the inventory for the controller (server only). Checks the policy's openable rule, reach and the before-open veto. */
	ERockOpenResult Open(const AController* Controller, const URockInventory* Inventory);
	/** Closes one open. False if it was not open. */
	bool Close(const AController* Controller, const URockInventory* Inventory, ERockCloseReason Reason = ERockCloseReason::Requested);
	/** Closes everything the controller has open. Returns how many. */
	int32 CloseAll(const AController* Controller);

	bool IsOpen(const AController* Controller, const URockInventory* Inventory) const;
	TArray<const URockInventory*> GetOpenInventories(const AController* Controller) const;
	TArray<const AController*> GetViewers(const URockInventory* Inventory) const;

	/** Closes every open whose controller is out of reach, and forgets entries whose controller or inventory is gone. Called on a timer from Tick. */
	void RecheckReach();

	/** Registers the container's rules, replacing earlier ones. */
	void SetPolicy(const URockInventory* Inventory, FRockInventoryAccessPolicy Policy);
	void ClearPolicy(const URockInventory* Inventory);
	/** The registered policy, or a default one. */
	FRockInventoryAccessPolicy GetPolicy(const URockInventory* Inventory) const;

	/** Shared reason (stub): gives one controller fixed rights on one inventory until RevokeShared. Needs no reach. */
	void GrantShared(const URockInventory* Inventory, const AController* Controller, ERockInventoryRights Rights);
	void RevokeShared(const URockInventory* Inventory, const AController* Controller);

	/** Raised before an open is accepted. */
	FRockInventoryBeforeOpen OnBeforeOpen;
	/** Raised once an open was accepted (not for AlreadyOpen). */
	FRockInventoryAfterOpen OnAfterOpen;
	/** Raised when an open ends while both parties still exist. */
	FRockInventoryClosed OnClosed;

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	/** Whether the controller's pawn is close enough to keep the container open. Override or set a policy for other rules. */
	virtual bool IsWithinReach(const AController& Controller, const URockInventory& Inventory, const AActor& InventoryActor, const FRockInventoryAccessPolicy& Policy) const;

private:
	struct FOpenEntry
	{
		TWeakObjectPtr<const AController> Controller;
		TWeakObjectPtr<const URockInventory> Inventory;
	};
	struct FSharedGrant
	{
		TWeakObjectPtr<const AController> Controller;
		TWeakObjectPtr<const URockInventory> Inventory;
		ERockInventoryRights Rights = ERockInventoryRights::None;
	};

	static const AActor* GetOwningActorOf(const URockInventory* Inventory);
	/** The inventory sits on this controller, its pawn or its player state. */
	static bool IsPersonal(const AController& Controller, const AActor& InventoryActor);
	const FRockInventoryAccessPolicy* FindPolicy(const URockInventory* Inventory) const;
	bool IsOpenEntry(const AController* Controller, const URockInventory* Inventory) const;

	TArray<FOpenEntry> OpenEntries;
	TArray<FSharedGrant> SharedGrants;
	TMap<TWeakObjectPtr<const URockInventory>, FRockInventoryAccessPolicy> Policies;
	float SinceRecheck = 0.f;
};
