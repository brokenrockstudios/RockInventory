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
- [Other Great Inventory Systems](#other-great-inventory-systems)
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
| `Generation`      | 8-bit counter used to invalidate stale `FRockItemStackHandle` references without shrinking the backing array                    |
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
// delegates to Definition 
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
| `NestedInventory`           | Optional child `URockInventory` (replicated); populated from `Definition->InventoryConfig` |

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
// Fragments can veto stack merging virtual bool CanCombineItemStack(const FRockItemStack& A, const FRockItemStack& B)
const;

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
│  AddItem / Transaction
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
2. At runtime, `URockInventoryLibrary::AddItem` creates an `FRockItemStack` referencing that definition and inserts it
   into `ItemData`.
3. If the definition requires a runtime instance, `URockItemInstance` (or a game-specific subclass) is spawned and
   linked to the stack.
4. The `FRockInventoryItemContainer` delta-replicates the stack array to clients; `URockItemInstance` replicates
   separately via the standard `UObject` replication path.
5. Code that needs to react to changes listens to `URockInventory::OnSlotChanged` / `OnItemStackChanged` delegates.

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

## Credit

Special acknowledgement and sincere gratitude to MajorTomAW (https://github.com/MajorTomAW) for their fantastic work on
the ItemizationCore system,
and for letting me borrow snippets and ideas as reference from it. Their work has helped accelerate this project's early
development.









