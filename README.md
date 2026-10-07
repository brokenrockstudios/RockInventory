> [!CAUTION]
> This Repo is under ongoing construction/refactor/redesign.

# RockInventory

Highly opinionated inventory used for BRS games.

BRS has used variety of existing inventory systems for various periods of time but never created an original inventory
system.
Game Inventories system are often game specific with plenty of opinions about game design and sometimes genre specific.

Feel free to use, learn from, reference, enjoy, or contribute!

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
    - [Design Philosophy](#design-philosophy)
    - [The Core Trio](#the-core-trio)
        - [URockItemDefinition: The Blueprint](#urockitemdefinition-the-blueprint)
        - [FRockItemStack: The Workhorse](#frockitemstack-the-workhorse)
        - [URockItemInstance: The Escape Hatch](#urockiteminstance-the-escape-hatch)
    - [Fragment System](#fragment-system)
    - [URockInventory: The Container](#urockinventory--the-container)
    - [Data Flow](#data-flow)
- [Pickup placement](#pickup-placement)
- [Other Great Inventory Systems](#other-great-inventory-systems)
- [Development with agents](#development-with-agents)
- [Credit](#credit)

---

## Architecture Overview

### Design Philosophy

RockInventory is built around a **struct-first** design. The guiding principle is:

> Keep items as lightweight, replication-friendly value types for as long as possible. Escalate to a full `UObject` only
> when you truly need runtime state.

This keeps the common case (a stack of arrows, a bag of gold) cheap. No heap allocation per item, no per-item GC
pressure, fast `FFastArraySerializer` delta replication. The `UObject` escape hatch (`URockItemInstance`) exists for the
cases that genuinely need it: a nested backpack inventory, a weapon with mutable mod sockets.

---

### The Core Trio

---

#### `URockItemDefinition`: The Blueprint

**File:** `Public/Item/RockItemDefinition.h`

`URockItemDefinition` is a `UPrimaryDataAsset`. It is the **immutable source of truth** for everything a designer
configures about an item. It is loaded by the Asset Manager and shared across all instances of that item type. Nothing
in it should change at runtime.

Key responsibilities:

| Property group     | What it holds                                                                                                  |
|--------------------|----------------------------------------------------------------------------------------------------------------|
| **Identity**       | `ItemId` (FName), `Name`, `DisplayName`, `Description`                                                         |
| **Inventory**      | `MaxStackCount`, `GridSize` (FIntPoint for grid-based layouts)                                                 |
| **Classification** | `ItemType` (tag container), `ItemSubType`, `ItemRarity`, `ItemTags`                                            |
| **Stats**          | `StatTag`                                                                                                      |
| **World**          | `ItemMesh`, `ItemSkeletalMesh`, `ActorClass` (soft references, streamed via Asset Bundles)                     |
| **Behaviour**      | `bRequiresRuntimeInstance`, `RuntimeInstanceClass`, `InventoryConfig` (for nested inventories e.g. a backpack) |
| **Fragments**      | `TArray<FRockItemFragmentInstance>` composable, instanced structs                                              |

The definition deliberately uses `TSoftObjectPtr` / `TSoftClassPtr` throughout so assets are only loaded into memory
when the game needs them (UI bundle, gameplay bundle, etc.).

> **Contributor note:** `StatTagDefaults` (editor-only `TArray<FGameplayTagStack>`) are flattened into the transient
`StatTags` container by `RebuildStatTags()` on `PostLoad` and `PostEditChangeProperty`. This two-step exists because
`FGameplayTagStackContainer` cannot hold UPROPERTY default values directly.

---

#### `FRockItemStack`: The Workhorse

**File:** `Public/Item/RockItemStack.h`

`FRockItemStack` is the **runtime representation of an item sitting in a slot**. It is a plain `USTRUCT` that extends
`FFastArraySerializerItem`, making it a first-class citizen of Unreal's delta-replication system.

Key members:

| Member            | Purpose                                                                                                                         |
|-------------------|---------------------------------------------------------------------------------------------------------------------------------|
| `Definition`      | Pointer to the owning `URockItemDefinition`                                                                                     |
| `StackCount`      | How many of the item are in this stack                                                                                          |
| `CustomValue1/2`  | Generic integer payload. Such as durability, charge level, ammo count. Meaning declared by `CustomValue1Tag` on the definition) |
| `RuntimeInstance` | Nullable pointer to a `URockItemInstance`; `nullptr` for the vast majority of items                                             |
| `Generation`      | 16-bit counter used to invalidate stale `FRockItemStackHandle` references without shrinking the backing array                    |
| `bInitialized`    | Guards one-time setup (fragment `OnItemCreated` callbacks, runtime instance spawning)                                           |

Write access to `FRockItemStack` internals is intentionally **restricted via `friend` declarations** to
`URockInventory`, `URockItemStackLibrary`, and a small number of trusted types. This ensures that mutations always flow
through the inventory system so the Fast Array serializer can mark items dirty for replication and broadcast appropriate
events.

```cpp
// The stack carries just enough to be self-describing 
FName Id = Stack.GetItemId(); 
int32 Count = Stack.GetStackCount(); 
int32 MaxCount = Stack.GetMaxStackCount(); 
// same definition and custom values, no two different runtime instances, and no fragment veto
bool CanMerge = Stack.CanStackWith(OtherStack);
```

The `FRockItemStackHandle` companion struct gives you a stable, generation-checked reference to a stack inside
`FRockInventoryItemContainer::AllSlots` without holding a raw pointer.

---

#### `URockItemInstance`: The Escape Hatch

**File:** `Public/Item/RockItemInstance.h`

`URockItemInstance` is a **replicated `UObject`** that is allocated only when a definition sets the class. Think of it
as the item "coming alive". It can hold mutable per-item state, participate in the GC and replication graphs, and carry
a nested inventory.

When do you need it?

- The item has **mutable stats** that differ per-instance
- The item **contains its own inventory** (backpack, toolbox, loot chest)
- The item needs to run **server-authoritative logic** between ticks that a fragment callback cannot express

Key members:

| Member                      | Purpose                                                                                    |
|-----------------------------|--------------------------------------------------------------------------------------------|
| `OwningInventory`           | Back-reference to the `URockInventory` that holds this instance                            |
| `SlotHandle` / `ItemHandle` | Replicated handles so the instance can locate itself in the grid                           |
| `CachedDefinition`          | Quick access to the definition without going through the stack                             |
| `Tags`                      | Per-instance `FGameplayTagContainer`                                                       |
| `StatTags`                  | Per-instance `FGameplayTagStackContainer`. Mutable, unlike the definition's read-only copy |
| `NestedInventory`           | Optional child `URockInventory` (replicated); populated by `FRockItemFragment_NestedInventory` |

`URockItemInstance` is intentionally kept as a **base class**. You might subclass it (pointed to by
`RuntimeInstanceClass` on
the definition) when your game needs item-specific logic. Though we might eventually add a fragment system to the
instance in the future.

```cpp
URockItemInstance* Instance = Stack.GetRuntimeInstance(); 
if (Instance) { 
    Instance->AddStatTagCount(Tag_Durability, -1); 
}
```

> **Key insight:** If you never set the `RuntimeInstance` class, your item pays zero `UObject` cost. It lives entirely
> as a struct inside the replicated `TArray<FRockItemStack>`.

---

### Fragment System

**File:** `Public/Item/RockItemFragment.h`

Fragments are the **composition mechanism** for item behaviour. Rather than subclassing `URockItemDefinition` for every
item variant, you add one or more `FRockItemFragment`-derived structs to a definition's `Fragments` array.

They are stored as `TInstancedStruct<FRockItemFragment>` inside `FRockItemDefinition`, which means:

- **No `UObject` allocation**. Fragments are plain structs embedded in the definition asset.
- **Type-safe retrieval** via `Definition->GetFragmentOfType<T>()`.
- Fully supported in the editor via `ShowOnlyInnerProperties`.

Fragments have two engine-level hooks:

// Called once when an FRockItemStack is first initialized virtual void OnItemCreated(FRockItemStack& ItemStack) const;
// Fragments can veto stack merging: FRockItemStack::CanStackWith asks every fragment of the definition, and one false is final
virtual bool CanCombineItemStack(const FRockItemStack& A, const FRockItemStack& B) const;

Built-in examples: `FRockItemFragment_SetStats` (seeds `CustomValue1/2` on creation), `FRockItemFragment_Actor` (world
actor data), `FRockItemFragment_FuelData`.

---

### `URockInventory`: The Container

**File:** `Public/Inventory/RockInventory.h`

`URockInventory` is a `UObject` that owns:

- **`FRockInventoryItemContainer`**: a `FIrisFastArraySerializer`-backed `TArray<FRockItemStack>` (the actual item
  data, replicated).
- **`FRockInventorySlotContainer`**: the grid slot metadata (which stack lives at which `(tab, row, col)` coordinate).
- **`TArray<FRockInventorySectionInfo>`**: tab/section configuration (each section is a named grid of W×H slots).

The separation of *item data* from *slot data* is deliberate: an item occupying multiple grid cells needs only one
`FRockItemStack` entry, with one slot entries pointing at it.
The separation allows for a more efficient representation of the inventory, allowing items to freely move around
internal to the inventory with causing the tiniest possible replication overhead. Mostly just a 4 byte ItemHandle.

The `FreeIndices` stack enables O(1) item slot recycling without compacting the array (which would invalidate replicated
handles).

Mutations are exposed through `URockInventoryLibrary` / `URockItemStackLibrary` function libraries (and `Transactions/`)
rather than directly on the component, keeping the replication marking logic in one place.

---

### Data Flow

```
Designer authors URockItemDefinition
│
│ (Asset Manager. Loaded on demand)
│
│  LootItemToInventory / Transaction
▼
URockInventory::ItemData  [ FRockItemStack, FRockItemStack, ... ]
│                               │
│ slot grid                     │ if URockItemInstance class set
▼                               ▼
URockInventory::SlotData        URockItemInstance
│                               │
▼                               ▼
FFastArray delta replication    Iris / standard UObject replication
```

1. A designer creates a `URockItemDefinition` asset and optionally adds fragments.
2. At runtime, `URockInventoryLibrary::LootItemToInventory` (or `URockInventory::AddItemToInventory` for a known slot) creates an `FRockItemStack` referencing that definition and inserts it
   into `ItemData`.
3. If the definition requires a runtime instance, `URockItemInstance` (or a game-specific subclass) is spawned and
   linked to the stack.
4. The `FRockInventoryItemContainer` delta-replicates the stack array to clients; `URockItemInstance` replicates
   separately via the standard `UObject` replication path.
5. Code that needs to react to changes listens to `URockInventory::OnChangeBatch`: one `FRockInventoryChangeBatch` (slot deltas, item deltas, replicated `Revision`) per finished operation on the server and per replication update on a client, so a move is one batch holding both slots. `OnSlotChanged` / `OnItemChanged` still exist as adapters replayed from the batch. Code that changes several things at once can group them with `FRockInventoryOperationScope`.

---

## Plain-data moves

`FRockInventoryData` is the inventory as plain data (sections, slots, stacks) with no world, events or authority. `FRockInventoryData::CanMove` answers whether a move, merge, split, rotation or cross-inventory transfer is allowed, and `ApplyMove` does it and returns an `FRockInventoryChangeSet` of before/after slots and stacks. `URockInventoryLibrary::MoveItem` snapshots the inventories into it, applies the move and commits the change set, so a move behaves the same on the server, in a test and (later) in a client's prediction model.

The same data has `CanAdd`/`ApplyAdd` (put a stack at one slot: a new stack in an empty cell that fits, or a top-up of a stack it stacks with) and `CanRemove`/`ApplyRemove` (take some or all of the stack at a slot), plus `CountMatching` and `RemoveMatching` (remove up to N items from the stacks a predicate accepts, lowest slot first, optionally all-or-nothing). `URockInventoryLibrary::AddItemToSlot`, `RemoveMatching` and `RemoveItemsById` run them on a snapshot and commit the change set; the commit is where runtime instances are created and `OnItemCreated` runs. Pickup placement (`LootItemToInventory`) has its own planner on the live inventory and does not use these yet.

## Access

Every server command asks `URockInventoryManagerComponent::CanAccess`, which asks the world's `URockInventoryAccessSubsystem`. Deny by default. A player has access to an inventory when it sits on their own controller, pawn or player state (always), when they opened it and are still within reach (`Server_OpenInventory` / `Server_CloseInventory`; a periodic check closes the open when they walk away), when they own the container and are nearby (view only), or through a per-container grant. A player's own inventory cannot be opened by others; a chest, a body or a dropped backpack can, by anyone in reach, and several players may have the same one open. Per-container rules (who may open, reach mode, owner) are set with `SetPolicy`; `OnBeforeOpen` can veto, `OnAfterOpen` and `OnClosed` announce. Access is an answer with rights (None, View, LimitedTake, Full), so a later rule can answer with less than Full.

## Replication scope

An inventory and its item instances replicate only to the connection that owns the actor and to players the access registry gives View rights (an open within reach, a container they own nearby, a shared grant); everyone else gets nothing, not even an empty shell. The registry puts the objects in each viewer's private net group and takes them out on close, on walking out of reach and on a revoked grant. A nested inventory is its own unit by default (`Visibility = Separate` on the inventory config): opening the chest shows the backpack item, opening the backpack shows what is inside. Set `Visibility = FollowsParent` for a weapon's attachment inventory: whoever sees the gun sees the scope, with the gun's access and sync state. Set `Visibility = OwnerOnly` for a secure container: everyone sees the item, only its player ever receives or may touch the contents (opens and shared grants are refused), even when the body is looted. Set `RockInventory.GatedReplication 0` to replicate to everyone as before.

A closed inventory is not destroyed on the client; it keeps its last contents and stops updating. `URockInventory::GetSyncState()` tells the UI which case it is: `Unknown` (never granted), `Syncing` (granted, state not caught up), `Live`, `Stale` (the grant ended), with `OnSyncStateChanged` when it changes. Anything on a remote client that needs another player's item (weapon cosmetics, spectating) must read replicated equipment state, not the inventory.

## Pickup placement

Where a picked-up item lands is decided by data on the sections of the inventory config, not by code. One planner
(`URockInventoryLibrary::BuildLootPlan`) serves `LootItemToInventory` and the read-only `PreviewLoot`, so a preview
always matches the real call. `DescribeLootPlan` prints the section order and why each other section was skipped.

### Section settings

| Setting (on `FRockInventorySectionInfo`) | Meaning |
|---|---|
| `SectionFilter` | Hard rule: only items whose tags match may be placed here (by pickup or by drag). |
| `AcceptedLootIntents` | Which loot calls may use the section: `Store` (default), `Equip`, both, or none (an oven input that nothing should auto-fill). |
| `LootPriority` | Lower numbers are tried first (`-1` before `0` before `10`). Ties keep config order. |
| `LootPreference` | Soft rule: sections whose preference matches the item are tried before the rest. It only reorders, it never excludes. |

Order is (preference match, priority, config order). Placement is merge first: partial stacks in any usable section are
topped up before an empty slot is used, then the remainder goes to one new stack in the first slot that fits (rotated
if needed). Dragging an item onto a chosen slot is not loot and ignores intents, priority and preference.

### Intents, equip and swap

| Call `Intent` | What it can do |
|---|---|
| `Store` | Uses only sections that accept `Store`. Never equips and never swaps. |
| `Equip` | Uses only sections that accept `Equip`. With `bAllowSwap` and no empty equipment slot, it displaces the first occupied slot (plan order) the item fits in; the displaced stack is stored like a `Store` call, and the whole call is refused with nothing changed when it cannot be stored. |
| `Store \| Equip` (default for a plain pickup) | Both kinds of section, by the order above. Never swaps. |

An `Equip` call places the item in an equipment section; it does not make the character wear it. That is the game's
equipment code (for example sections tagged `AutoEquip`).

### Example: sword, pistol and pot

A player config of Head, Primary, Secondary and Backpack sections:

| Section | Filter | `AcceptedLootIntents` | `LootPriority` | `LootPreference` |
|---|---|---|---|---|
| Head | Headgear | `Equip` | 0 | none |
| Primary | Weapon and Wieldable | `Equip` | 0 | Weapon and not Sidearm |
| Secondary | Weapon | `Equip` | 1 | Sidearm |
| Backpack | none | `Store` | 10 | none |

With a `Store | Equip` call (these are the cases in `BRS.RockInventory.Loot.Priority`):

- A sword goes to Primary, the next sword to Secondary, the next to the Backpack.
- A pistol prefers Secondary, then falls through to Primary (allowed, just not preferred), then the Backpack.
- A pot (Headgear and Wieldable) goes to Head, then Primary, then the Backpack (Secondary wants a weapon).

An `Equip`-only call with `bAllowSwap` for a sword, when every equipment slot that takes it is occupied, swaps with
the first of them in plan order and stores the old item in the Backpack (`BRS.RockInventory.Loot.Swap`). An empty
equipment slot is always used before a swap.

### Validation

`URockInventoryConfig::IsDataValid` (shown by the data validator in the editor) warns, without failing the asset, about:

- no section with slots that accepts `Store` (a plain pickup could never place anything);
- two sections with the same section tag;
- a `LootPreference` that no item passing the section's `SectionFilter` could match (for example it asks for a tag the
  filter excludes). Matching is checked by trying every combination of the tags the two queries mention; beyond 10
  distinct tags the check is skipped.

A filter that merely lists other tags is not flagged: one item can carry both tags. `URockInventoryConfig::CollectLootIssues`
runs the same checks on any section list, which is what the tests use.

### Setup for Fen

The player inventory config: equipment sections (Head, Primary, Secondary, ...) get `AcceptedLootIntents = Equip`,
storage sections (pockets, backpack) keep `Store` with a larger `LootPriority` than the equipment sections, and
`LootPreference` on Primary and Secondary as in the table above. Check that the Primary and Head filters admit the items
you intend. Equipment sections that carry `AutoEquip` still wear items through the equipment manager, not through the
intent.

---

## Other Great Inventory Systems

There are many other great inventory systems out there.

* Lyra's Inventory System by Epic (Unreal Engine)
    * Supposedly inspired by UEFN's inventory system
    * ItemFragment system concept is great.
* ArcInventory by Puny Human and RoyAwesome
    * A favorite of mine which is heavily inspired from some of Lyra's core design.
    * Highly recommend if you want general purpose inventory. Prototype friendly!
    * Efficient minimal opinion a super solid backend.
* InventorySystemX by SixLine Studio
    * This has some nice UI/UX aspect.
* InventoryFrameworkPlugin by Varian Daemon
    * This has a neat custom shaped items.
      Other notable mentions
* RPG Inventory Template
* Inventory Grid by LucasBastos
* Action Rpg Inventory System by VAnguard interactive

Heavily influenced designs by games like Diablo, Path of Exile, Escape from Tarkov, Subnautica, Minecraft, Dyson Sphere
Program, and many more.

## Development with agents

This repo is developed through collaboration between people and AI coding agents, using modern tools and workflows.
Agents help with code, reviews, and bug fixes, and maintainers guide the direction and own the result. Maintainers read
every diff before it merges.

Anyone, human or agent, is welcome to contribute. Contributors are responsible for what they submit, so please avoid
low-effort or drive-by changes. Make sure that a change has been thought through and fits the project's design, and
consider everyone who might use or maintain it, not just the one case in front of you.

We acknowledge this collaboration here, at the project level, rather than stamping it on individual commits or PRs.
See [AGENTS.md](AGENTS.md) for the guidelines agents follow when contributing.

## Credit

Special acknowledgement and sincere gratitude to MajorTomAW (https://github.com/MajorTomAW) for their fantastic work on
the ItemizationCore system,
and for letting me borrow snippets and ideas as reference from it. Their work has helped accelerate this project's early
development.









