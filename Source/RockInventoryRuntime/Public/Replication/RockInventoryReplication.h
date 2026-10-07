// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryReplication.generated.h"

class APlayerController;
class URockInventory;

/** What a client knows about the contents of an inventory it holds a copy of. */
UENUM(BlueprintType)
enum class ERockInventorySyncState : uint8
{
	/** Never granted: the client holds no data for it (or only an empty shell). A UI shows "searching" or nothing. */
	Unknown,
	/** Granted by the server, but the replicated state has not caught up with the grant yet. A UI shows "searching". */
	Syncing,
	/** Granted and caught up: the contents are current, and updates keep arriving. */
	Live,
	/** The grant ended (closed, out of reach): the copy is kept but no longer updates. Show it greyed out or clear it. */
	Stale,
};

/**
 * Who sees a nested inventory (an item's own inventory) when they can see the inventory holding the item. Only about who is a viewer; when a
 * grant becomes visible (instant open or a timed search) is a property of the container's access policy, not of this.
 */
UENUM(BlueprintType)
enum class ERockNestedVisibility : uint8
{
	/** A container of its own: it replicates only to players who open it (a backpack's contents). */
	Separate,
	/** Seen by everyone who sees the item, with the parent's access and sync state (a weapon's attachments, a scope on a gun). */
	FollowsParent,
	/**
	 * Only the player the item sits on ever receives or may touch the contents, whoever opens the parent, shares it, or loots the body (a
	 * secure container). Others see the item, never what is inside. Opens and shared grants on it are refused.
	 */
	OwnerOnly,
};

/** One inventory the server replicates to this client, and the server's Revision when it started. Replicated to the owner only. */
USTRUCT()
struct ROCKINVENTORYRUNTIME_API FRockObservedInventory
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<URockInventory> Inventory;

	/** The inventory's Revision when the grant was made. The client is Live once its replicated Revision reaches this. */
	UPROPERTY()
	uint32 Revision = 0;
};

/**
 * Gated replication (T-76). Inventories and their item instances are registered with COND_NetGroup, so a client receives them only
 * through the group NetGroupOwner (the connection that owns the actor) or the private viewer group of its player controller. Who is in
 * which group is decided by URockInventoryAccessSubsystem (owner, open, proximity, shared). Nothing is registered with COND_None while
 * RockInventory.GatedReplication is 1 (the default).
 *
 * A gating unit is one inventory plus the item instances inside it. A nested inventory is its own unit (its own viewers), although the
 * instance that holds it belongs to the unit of the inventory the item is in.
 */
namespace RockInventoryReplication
{
	/** RockInventory.GatedReplication. When 0 every registration is COND_None, as before T-76. */
	ROCKINVENTORYRUNTIME_API bool IsGatingEnabled();

	/** The private net group of a player controller. One FName per player, not per container. */
	ROCKINVENTORYRUNTIME_API FName GetViewerGroup(const APlayerController& Controller);

	/**
	 * Registers SubObject on its replication owner (an actor or an actor component: TopLevelOwner) and, when gating is on, puts it in
	 * the groups GatingInventory currently has: NetGroupOwner plus the viewer group of every viewer. A null GatingInventory means owner only.
	 * Returns false when TopLevelOwner is neither an actor nor a component.
	 */
	ROCKINVENTORYRUNTIME_API bool RegisterSubObject(UObject* TopLevelOwner, UObject* SubObject, URockInventory* GatingInventory);

	/** Undoes RegisterSubObject: removes the registration and every group membership. */
	ROCKINVENTORYRUNTIME_API void UnregisterSubObject(UObject* TopLevelOwner, UObject* SubObject);

	/** Makes SubObject's group memberships exactly Groups (adds, removes, tells the replication system). Server only. */
	ROCKINVENTORYRUNTIME_API void SetSubObjectGroups(const UObject* WorldContext, UObject* SubObject, TConstArrayView<FName> Groups);
}
