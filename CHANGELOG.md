# Changelog

Version format `YYMM.DDRR` (year, month, day, revision of that day). Newest first. Keep entries to one line where possible.

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
