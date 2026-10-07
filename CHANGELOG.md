# Changelog

Version format `YYMM.DDRR` (year, month, day, revision of that day). Newest first. Keep entries to one line where possible.

## 2610.0702
- Added (T-76): gated replication. Inventories and item instances register with `COND_NetGroup` (`RockInventoryReplication::RegisterSubObject`) in `NetGroupOwner` plus the private viewer group (`RockViewer_<id>`) of every player with View rights; `URockInventoryAccessSubsystem` recomputes viewers on every open, close, grant, policy change and reach re-check (proximity too) and moves the inventory and its instances in and out of the groups (`RefreshReplication`). A nested inventory is its own unit; the backpack item follows the inventory it is in. Console variable `RockInventory.GatedReplication` (default 1; 0 = COND_None as before).
- Changed (T-76): `RegisterReplicationWithOwner` of inventories and instances no longer publishes to everyone first. Chests, bodies and other players' inventories now reach a client only through an open (`Server_OpenInventory`), proximity or a shared grant: until T-131 calls the open from gameplay they do not appear on other clients.
- Fixed (T-76): the engine only ever adds an object to an Iris net group, so a closed viewer kept receiving updates; the object is now removed from the group in Iris too.
- Added (T-76): `URockInventoryConfig::Visibility` (`ERockNestedVisibility`, copied to the inventory by `Init`; `Separate`, the default, `FollowsParent`, or `OwnerOnly`: a secure container whose contents only the player it sits on ever receives or may touch; opens, shared grants and proximity are refused or ignored). A `FollowsParent` nested inventory (a scope on a gun) is gated, accessed and sync-stated with the inventory holding the item: its instances and its own `FollowsParent` children join the parent viewers' groups (`URockInventory::GetGatingRoot`, `ForEachGatedObject`), `CanAccess`/`Open`/`Close` resolve to the root, and the `Observed` entry is the root's.
- Added (T-76): client sync state `URockInventory::GetSyncState()` / `OnSyncStateChanged` (Unknown, Syncing, Live, Stale), from the owner-only replicated `URockInventoryManagerComponent` `Observed` list (inventory + Revision at the grant).
- Added (T-76): `BRS.RockInventory.Gating` (groups per viewer, instances, nested inventories, proximity, shared, moves, observed list, gating off) and `Network.RockInventory.Gating` (PIE, `-Network`: second client sees nothing, sees a chest and its instance only while it has it open, a nested inventory only after opening it, goes Stale after close or lost reach).
- Changed (tests): `Network.RockInventory.NetGroupSpike.ItemInstance_NeedsTheSameMembershipAsItsInventory` now expects 0 leaked instances (it expected 1 because the plugin's own registration used COND_None).

## 2610.0701
- Fixed (T-127): a client copy of an inventory that arrives as a plain replicated subobject (nested inventories, inventories gated per viewer) never set `OwnerInventory` on its slot and item arrays, so the array callbacks returned early: no `OnChangeBatch`, no legacy deltas, and the item-to-slot index was not updated by replication. `URockInventory::PostInitProperties` now sets it for every non-CDO inventory (`URockInventoryComponent::OnRep_Inventory` did it only for the top-level one).
- Added (T-127): `Network.RockInventory.ChangeSet` (PIE, `-Network`): the owner and a granted viewer each get one `OnChangeBatch` per replication update carrying the replicated `Revision` (two operations in one server step: one batch, revision +2), the legacy deltas replay from it, and the item-to-slot lookup follows the move.

## 2610.0606
- Changed (T-124): the loot commit (`CommitLoot`, swap included) and `SplitItemStackAtLocation` go through the Layer 0 `ApplyAdd`/`ApplyRemove` and one `CommitChangeSet`; occupancy has one implementation (`FRockInventoryData::FillOccupancy`/`MarkFootprint`, used by `PrecomputeOccupancyGrids` too).
- Changed (T-124): looting more than one stack's worth into empty slots now makes several stacks of at most the max size (before: one stack above the max); a swap is refused for a count above the max. A whole-stack split records the slot change before the stack removal.
- Added (T-124): `Loot_MoreThanOneStackIntoEmptySlots_SplitsAtTheMaxStackSize`, `Loot_MoreThanFitsInEmptySlots_ReportsTheRestAsExcess`.

## 2610.0605
- Fixed (tests): `SpawnPawnAt` was defined in two test files' anonymous namespaces and collided in unity builds; moved to `RockInventoryTestFixture.h`. Replaced deprecated bool `GetObjectsWithOuter` calls with `EGetObjectsFlags`.

## 2610.0604
- Added (T-75): `URockInventoryAccessSubsystem`, the server-side access registry. Reasons Owner, Open, Proximity (view only, for containers you own) and Shared (stub); rights None / View / LimitedTake / Full; per-container `FRockInventoryAccessPolicy` (openable by, reach mode Default / Ignore / Custom, reach override, owner); C++ delegates `OnBeforeOpen` (veto), `OnAfterOpen`, `OnClosed`; periodic reach re-check. Deny by default.
- Added: `URockInventoryManagerComponent::Server_OpenInventory` / `Server_CloseInventory`; the component closes its controller's opens in `EndPlay`.
- Changed: `CanAccess(Inventory, Instigator, Required = Full)` is a plain C++ virtual (no longer a Blueprint event) and asks the registry: another player's container, in reach or not, now needs an explicit open, and a player's own inventory cannot be opened by others. `MaxAccessReach` moved to `URockInventoryAccessSubsystem::DefaultReach`.
- Added: `BRS.RockInventory.Access` tests; `ServerCommands` tests open the container first and gained a never-opened case.
## 2610.0603
- Added (T-74): change sets. `URockInventory::OnChangeBatch` delivers one `FRockInventoryChangeBatch` (slot deltas, item deltas, `Revision`) per finished server operation (move, loot, split, setter call; `FRockInventoryOperationScope` groups calls) and per replication update on a client (flushed in `PostNetReceive`). A move inside one inventory is one batch holding both slots.
- Added: replicated `URockInventory::Revision` (`GetRevision()`), raised once per server operation that changed something.
- Changed: `OnSlotChanged` and `OnItemChanged` are adapters replayed from the batch, in the order the changes were made, after the operation is complete (they used to fire in the middle of it). `GetSlotByItemHandlePtr` / `GetSlotByItemHandle` are O(1) through an item-to-slot index kept on the server and on clients.
- Added: `BRS.RockInventory.ChangeSet` tests (batches, revision, client flush, randomized index-versus-scan).
## 2610.0602
- Added (T-73): Layer 0 `FRockInventoryData::CanAdd`/`ApplyAdd`, `CanRemove`/`ApplyRemove`, `CountMatching` and `RemoveMatching` (all-or-nothing option, lowest slot first), with `ERockAddRefusal`/`ERockRemoveRefusal`. `URockInventoryLibrary::AddItemToSlot`, `RemoveMatching`, `CountMatching` and `RemoveItemsById` (BlueprintAuthorityOnly) run them and commit the change set.
- Changed: `FRockItemStack::CanStackWith` asks every fragment's `CanCombineItemStack` (a veto is final) and refuses two different runtime instances; loot merges and moves respect it.
- Removed: `FRockItemDefinitionFragment`, `RockContainerExperiment`, `FRockInventoryChangeEvent`, `ERockInventoryChangeType`, `ERockTransactionState`, `ERockTransactionResult`, and the runtime module's public UMG dependency. `K2_DropItem` shows as "Drop Item"; `K2_HasItem` and `K2_GetItemCount` are BlueprintPure. README drift fixed.
- Added: `BRS.RockInventory.AddRemove` tests, add/remove cases in `BRS.RockInventory.Data`, fragment veto and instance cases in `BRS.RockInventory.Stack`.
## 2610.0601
- Added (T-72): `FRockInventoryData` (Layer 0, plain sections/slots/stacks, no UObject inventory) with side-effect-free `CanMove` and `ApplyMove` (move, merge, split, rotate, cross-inventory transfer) returning an `FRockInventoryChangeSet`; `ERockMoveRefusal` says why a move is refused.
- Changed: `URockInventoryLibrary::MoveItem` snapshots the inventories into `FRockInventoryData`, applies the move there and commits the change set (same events, same warnings). New `BRS.RockInventory.Data` tests run the Move scenarios on plain data.

## 2610.0512
- Docs (T-94): nested subobject replication answered in DevNotes (UE 5.8 has no public nested registry; `Owner` link and re-registration stay). No code change.

## 2610.0511
- Added (T-37): `URockInventoryConfig::IsDataValid` warns (never fails) about a config with no section that has slots and accepts `Store`, duplicate section tags, and a `LootPreference` no item admitted by the section's `SectionFilter` can match. The checks are the static `URockInventoryConfig::CollectLootIssues` (`FRockConfigIssue`, `ERockConfigIssue`).
- Added: `BRS.RockInventory.Config` tests, README "Pickup placement" section.

## 2610.0510
- Added (T-95): `URockInventoryLibrary::PreviewLoot(Inventory, TArray<FRockItemStack>, Params)` returns one `FRockLootResult` per stack, simulating them in order against a scratch copy of occupancy, partial stacks and the new stacks earlier ones would create (a "take all" accounts for earlier stacks); matches looting each stack in order. `bAllowSwap` is ignored in the batch.
- Added: `PreviewArray_*` tests in `BRS.RockInventory.Loot.Api`.

## 2610.0509
- Added (T-62): Equip and swap. An Equip-only call (`Equip` without `Store`) with `FRockLootParams::bAllowSwap` that finds no empty equipment slot displaces the first occupied slot (plan order) the item fits in; the displaced stack is stored through a `Store` call (merge first), and the call is refused with nothing changed when it cannot be stored. `Store | Equip` and `Store` never swap. `FRockLootResult` gains `bSwapped`, `DisplacedSlot`, `DisplacedPlacements`; `FRockLootParams::CanSwap()`. `PreviewLoot` predicts all of it.
- Changed: `DecideLoot` split into private `DecideMerges`, `DecideNewStack`, `DecideSwap`, `ApplyPlacements`; behavior without a swap unchanged.
- Added: `BRS.RockInventory.Loot.Swap` tests (empty slot before swap, swap stores displaced, merge of displaced, no room refused, unaccepted item refused, no swap for Store or Store|Equip or without `bAllowSwap`, item that fits no occupied slot).

## 2610.0508
- Added (T-36): pickup placement priority. `FRockInventorySectionInfo` gains `AcceptedLootIntents` (`ERockLootIntent` flags, default `Store`), `LootPriority` (lower first, ties keep config order) and `LootPreference` (soft tag query: matching sections are tried first, never exclusion), with chainable setters. `URockInventoryLibrary::BuildLootPlan` orders the sections a call may use (const, no allocation up to 16 sections, `FRockLootPlanEntry` inventory + section index) and `DescribeLootPlan` explains the order and every skipped section. `FRockLootParams::Intent` is now applied: a section is used only when it shares an intent bit with the call.
- Changed (T-36): loot is merge-first. `DecideLoot` tops up partial stacks across all planned sections before it places a new stack, so a later partial stack beats an earlier empty slot (previously merge and fill were interleaved per slot). `ERockLootIntent` moved to `Enums/RockLootIntent.h`.
- Added: `BRS.RockInventory.Loot.Priority` tests (sword, pistol, pot, occupied slot, Store and Equip isolation, no-intent section, priority and ties, merge-first, plan description, timing log).

## 2610.0507
- Changed (T-35): loot API shape, no routing change. `URockInventoryLibrary::LootItemToInventory` now takes `FRockLootParams` and fills an `FRockLootResult` (placements as `FRockSlotReference`, count, orientation, new-stack flag, plus the excess) instead of `OutHandle`/`OutExcess`; the private `FLootDecision` is replaced by that public result. `URockInventoryComponent::K2_AddItem`/`K2_LootItem` and `FRockLootWorldItemUndoTransaction` (`Result` replaces `TargetSlotHandle`/`Excess`) follow; `FRockLootWorldItemTransaction` gains `LootParams`; `ARockInventoryWorldItemBase::OnPickedUp` loots with `Store | Equip`. Blueprint pins of the two K2 functions change.
- Added (T-35): `ERockLootIntent` (`Store`, `Equip`, flags), `FRockLootParams` (intent, `bAllowSwap`, `ExcludeSectionMetaTags`; intent and swap are carried, applied from T-36 and T-62), and read-only `URockInventoryLibrary::PreviewLoot` that returns the placements the real call would make. `URockInventory::MakeSlotReference` is now `const`.
- Removed (T-35): the unused `FRockLootPhase`, `ERockSectionFillStrategy` and old `FRockLootParams` sketch in `RockInventoryQuery.h`.
- Added: `BRS.RockInventory.Loot.Api` tests.
## 2610.0506
- Fixed (T-71): a nested inventory follows its item. Added C++-only `URockInventory::SetOwner`; `URockItemInstance::SetOwningInventory` now uses it to update the nested inventory's owner (null in a world item), so a moved backpack re-registers for replication through its new owner instead of the old one. Added `BRS.RockInventory.Nested` tests.

## 2610.0505
- Changed (T-70): library mutators (`LootItemToInventory`, `SplitItemStackAtLocation`, `MoveItem`, `MergeItemAtGridPosition`, `SetCustomValue1/2`) run on the authority only: a client call logs a warning, changes nothing and fails; the Blueprint ones are `BlueprintAuthorityOnly`. `AddItemToInventory` ensures instead of `checkf` and returns an invalid handle off the authority.
- Removed (T-70): `bEnablePredictiveExecution`, `FRockInventoryTransactionRecord`, the client and server transaction histories and `ClearHistory` on `URockInventoryManagerComponent`. `MoveItem`/`DropItem`/`LootWorldItem` only validate and send the server RPC.
- Added: `BRS.RockInventory.Authority` tests.

## 2610.0504
- Added (T-68): `Network.RockInventory.NetGroupSpike` PIE network tests (dedicated server, two clients, Iris) showing `COND_NetGroup` gating of an inventory and its item instances: owner-only, per-viewer grant, revoke leaves a stale copy. Test-only; no runtime change. Run with `RunTests.ps1 -Filter Network.RockInventory -Network`.

## 2610.0503
- Changed (T-69): server commands (`Server_MoveItem`, `Server_DropItem`, `Server_LootWorldItem`, `Server_RegisterSlotStatus`, `Server_ReleaseSlotStatus`) no longer trust the client. The instigator is the component's owning controller (`GetOwningController`), and every inventory a command names must pass `URockInventoryManagerComponent::CanAccess` (BlueprintNativeEvent; default: own pawn, controller or player state, or within `MaxAccessReach` of the instigator's pawn, 500 cm). `AuthorizeServerCommand` is the single gate.
- Changed (T-69): `Server_RegisterSlotStatus` and `Server_ReleaseSlotStatus` lost their `Instigator` parameter (Blueprint callers must be re-wired).
- Added: `BRS.RockInventory.ServerCommands` tests.

## 2610.0502
- Changed (T-34): `URockInventoryLibrary::LootItemToInventory` now walks sections then slots, checks each section filter once, and splits into a read-only `DecideLoot` and an applying `CommitLoot` (no per-slot section lookup, no copies of slot entries, sections or stacks while deciding). No placement behavior change; all `BRS.RockInventory.Loot` tests pass unchanged.

## 2610.0501
- Added (T-33): chainable `FRockInventorySectionInfo::SetSectionFilter` / `SetMetaTags`; `URockItemDefinition::RebuildCachedTags` is now public. No placement behavior change.
- Added: `BRS.RockInventory.Loot.Sections` and `BRS.RockInventory.Sections` tests pinning section filters, meta tags, config-order placement and merge versus empty slot across sections.

## 2610.0401
- Changed: `URockItemRegistrySubsystem` is now a `UEngineSubsystem` (`GetInstance()` needs no world and no longer crashes without one). It builds lazily, waits for its loads, and refreshes additively after `MarkDirty()`; `RockInventoryEditor` marks it dirty on PIE start and item definition asset changes.
- Fixed: `GetAllDefinitions` always returned an empty array.
- Added: `BRS.RockInventory.ItemRegistry` tests.

## 2610.0302
- Fixed `URockItemRegistrySubsystem` skipping every item definition (and logging "Failed to initiate load") for the second and later game instances in a process, because `LoadPrimaryAsset` returns a null handle for assets that are already loaded.

## 2610.0301
- Added initial unit tests (`RockInventoryTests`).
