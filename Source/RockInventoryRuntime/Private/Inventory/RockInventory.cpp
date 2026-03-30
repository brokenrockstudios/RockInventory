// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Inventory/Events/RockSlotChangeType.h"
#include "Inventory/Events/RockSlotDelta.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"

URockInventory::URockInventory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// Fast - slots with section context for free
void URockInventory::ForEachSlotInSection(const TFunctionRef<bool(const FRockInventorySectionInfo&, const FRockInventorySlotEntry&)>& Func) const
{
	for (const FRockInventorySectionInfo& Section : SlotSections)
	{
		const int32 FirstIndex = Section.GetFirstSlotIndex();
		const int32 NumSlots = Section.GetNumSlots();
		for (int32 slotIndex = 0; slotIndex < NumSlots; ++slotIndex)
		{
			if (!Func(Section, SlotData[FirstIndex + slotIndex]))
				return;
		}
	}
}

void URockInventory::ForEachItemStack(const TFunctionRef<bool(const FRockItemStack&)>& Func) const
{
	for (const FRockItemStack& ItemStack : ItemData)
	{
		if (!Func(ItemStack))
		{
			break;
		}
	}
}

AActor* URockInventory::GetOwningActor()
{
	const UObject* Current = URockInventoryLibrary::GetTopLevelOwner(this);
	if (const AActor* Actor = Cast<AActor>(Current))
	{
		return const_cast<AActor*>(Actor);
	}
	else if (const UActorComponent* Comp = Cast<UActorComponent>(Current))
	{
		return Comp->GetOwner();
	}

	UE_LOG(LogRockInventory, Warning, TEXT("GetOwningActor - No valid owning actor found for inventory %s"), *GetName());
	return nullptr;
}

void URockInventory::Init(const URockInventoryConfig* config)
{
	if (!config)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - Invalid config object"));
		return;
	}
	if (config->InventoryTabs.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory tabs defined"));
		return;
	}

	RegisterReplicationWithOwner();
	// Set owner references for containers
	ItemData.SetOwningInventory(this);
	SlotData.SetOwningInventory(this);

	// Initialize the inventory data
	int32 totalInventorySlots = 0;
	SlotData.Empty();
	ItemData.Empty();

	SlotSections.Empty();
	// Reserve space for the inventory data
	SlotSections.Reserve(config->InventoryTabs.Num());

	for (int32 sectionIndex = 0; sectionIndex < config->InventoryTabs.Num(); ++sectionIndex)
	{
		FRockInventorySectionInfo NewTab = config->InventoryTabs[sectionIndex];
		NewTab.Initialize(totalInventorySlots, sectionIndex);

		SlotSections.Add(NewTab);
		totalInventorySlots += NewTab.GetNumSlots();
	}
	// ItemData will grow, and not be preallocated
	SlotData.SetNum(totalInventorySlots);
	if (SlotData.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory slots defined"));
		return;
	}

	for (int32 SectionIndex = 0; SectionIndex < SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& TabInfo = SlotSections[SectionIndex];
		const int32 TabOffset = TabInfo.GetFirstSlotIndex();
		const int32 NumSlots = TabInfo.GetNumSlots();

		for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
		{
			const int32 AbsoluteSlotIndex = TabOffset + SlotIndex;
			SlotData[AbsoluteSlotIndex].SlotHandle = FRockInventorySlotHandle(AbsoluteSlotIndex);
			SlotData[AbsoluteSlotIndex].ItemHandle = FRockItemStackHandle::Invalid();
			SlotData[AbsoluteSlotIndex].Orientation = ERockItemOrientation::Horizontal;
			SlotData[AbsoluteSlotIndex].bIsLocked = false;
		}
	}

	SlotData.MarkArrayDirty();
	ItemData.MarkArrayDirty();
}

const FRockInventorySectionInfo& URockInventory::GetSectionInfo(const FGameplayTag& SectionTag) const
{
	const int32 SectionIndex = GetSectionIndex(SectionTag);
	if (SectionIndex != INDEX_NONE)
	{
		return SlotSections[SectionIndex];
	}
	return FRockInventorySectionInfo::Invalid();
}


int32 URockInventory::GetSectionIndex(const FGameplayTag& SectionTag) const
{
	if (!SectionTag.IsValid()) { return INDEX_NONE; }

	for (int32 i = 0; i < SlotSections.Num(); ++i)
	{
		if (SlotSections[i].GetSectionTag() == SectionTag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}


const FRockInventorySectionInfo& URockInventory::GetSectionInfoBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	// Food for Thought: 
	// If we wanted to eliminate this loop, we could maintain a TArray<uint8> SlotIndexToSectionIndex; mapping
	// Which would be 1 byte per section (usually less than 40)
	for (const FRockInventorySectionInfo& Section : SlotSections)
	{
		if (Section.ContainsSlotHandle(InSlotHandle))
		{
			return Section;
		}
	}
	return FRockInventorySectionInfo::Invalid();
}

FRockInventorySlotEntry URockInventory::GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const int32 slotIndex = InSlotHandle.GetAbsoluteIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("[%hs] - Invalid slot index"), __FUNCTION__);
		return FRockInventorySlotEntry::Invalid();
	}
	return SlotData[slotIndex];
}

FRockItemStack URockInventory::GetItemBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const FRockInventorySlotEntry& Slot = GetSlotByHandle(InSlotHandle);
	return GetItemByHandle(Slot.ItemHandle);
}

FRockItemStack URockInventory::GetItemByHandle(const FRockItemStackHandle& InItemHandle) const
{
	const FRockItemStack* ItemPtr = GetItemByHandlePtr(InItemHandle);
	return ItemPtr ? *ItemPtr : FRockItemStack::Invalid();
}

const FRockItemStack* URockInventory::GetItemByHandlePtr(const FRockItemStackHandle& InItemHandle) const
{
	if (!InItemHandle.IsValid()) { return nullptr; }

	const int32 Index = InItemHandle.GetIndex();
	if (!ItemData.ContainsIndex(Index))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetItemByHandlePtr - Bad Item Index"));
		return nullptr;
	}

	const FRockItemStack& Item = ItemData[Index];
	if (Item.Generation != InItemHandle.GetGeneration()) { return nullptr; }

	return &Item;
}

FRockInventorySlotEntry URockInventory::GetSlotByItemHandle(const FRockItemStackHandle& InItemHandle) const
{
	const FRockInventorySlotEntry* SlotPtr = GetSlotByItemHandlePtr(InItemHandle);
	return SlotPtr ? *SlotPtr : FRockInventorySlotEntry::Invalid();
}

const FRockInventorySlotEntry* URockInventory::GetSlotByItemHandlePtr(const FRockItemStackHandle& InItemHandle) const
{
	if (!InItemHandle.IsValid()) { return nullptr; }

	const int32 index = InItemHandle.GetIndex();
	if (!ItemData.ContainsIndex(index))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotByItemHandlePtr - Invalid item index"));
		return nullptr;
	}
	const FRockItemStack& Item = ItemData[index];
	if (Item.Generation != InItemHandle.GetGeneration()) { return nullptr; }

	for (const FRockInventorySlotEntry& SlotEntry : SlotData)
	{
		if (SlotEntry.ItemHandle == InItemHandle)
		{
			return &SlotEntry;
		}
	}
	return nullptr;
}

void URockInventory::SetItemByHandle(const FRockItemStackHandle& InSlotHandle, const FRockItemStack& InItemStack)
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!ItemData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetItemByHandle - Invalid item index"));
		return;
	}
	FRockItemStack& ChangedItem = ItemData[slotIndex];
	ChangedItem.CopyDataFrom(InItemStack);
	ItemData.MarkItemDirty(ChangedItem);
	BroadcastItemChanged(InSlotHandle, ERockItemChangeType::Removed);
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlotEntry)
{
	const int32 slotIndex = InSlotHandle.GetAbsoluteIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("[%hs] - Invalid slot index"), __func__);
		return;
	}

	FRockInventorySlotEntry& ChangedSlot = SlotData[slotIndex];
	ERockSlotChangeType ChangeType = ERockSlotChangeType::None;

	const bool bOldItemValid = ChangedSlot.ItemHandle.IsValid();
	const bool bNewItemValid = InSlotEntry.ItemHandle.IsValid();
	if (!bOldItemValid && bNewItemValid)
	{
		ChangeType = ERockSlotChangeType::ItemAdded;
	}
	else if (bOldItemValid && !bNewItemValid)
	{
		ChangeType = ERockSlotChangeType::ItemRemoved;
	}
	else
	{
		const bool bItemChanged = ChangedSlot.ItemHandle != InSlotEntry.ItemHandle;
		const bool bPropertiesChanged = ChangedSlot.Orientation != InSlotEntry.Orientation || ChangedSlot.bIsLocked != InSlotEntry.bIsLocked;
		// Does the SlotHandle ever actually change? As far as I can tell it doesn't after initialization.

		if (bItemChanged)
		{
			ChangeType = ERockSlotChangeType::ItemChanged;
		}
		else if (bPropertiesChanged)
		{
			ChangeType = ERockSlotChangeType::PropertiesChanged;
		}
	}

	if (ChangeType != ERockSlotChangeType::None)
	{
		// SlotHandle shouldn't ever change. It only is a way to reference itself.
		//ChangedSlot.SlotHandle = InSlotEntry.SlotHandle;

		const FRockItemStackHandle PreviousItemHandle = ChangedSlot.LastKnownItemHandle;
		ChangedSlot.ItemHandle = InSlotEntry.ItemHandle;
		ChangedSlot.LastKnownItemHandle = InSlotEntry.ItemHandle;
		ChangedSlot.Orientation = InSlotEntry.Orientation;
		ChangedSlot.bIsLocked = InSlotEntry.bIsLocked;
		SlotData.MarkItemDirty(ChangedSlot);

		FRockSlotDelta slotDelta(this, InSlotHandle, ChangeType, PreviousItemHandle);
		BroadcastSlotChanged(slotDelta);
	}
}

//
// /**
// 	 * Add a new tab and initialize its slots
// 	 * @param SectionName - The unique identifier for the tab
// 	 * @param Width - The width of the tab in slots
// 	 * @param Height - The height of the tab in slots
// 	 * @return The index of the newly created tab
// 	 */
// int32 URockInventory::AddSection(const FName& SectionName, int32 Width, int32 Height)
// {
// 	FRockInventorySectionInfo NewTab(
// 		SectionName, SlotData.Num(), Width, Height);
// 	
// 	// NewTab.SectionName = SectionName;
// 	// NewTab.Width = Width;
// 	// NewTab.Height = Height;
// 	// NewTab.FirstSlotIndex = SlotData.Num(); // Current size is the first index
//
// 	const int32 TabIndex = SlotSections.Add(NewTab);
//
// 	// Reserve space for the new tab's slots
// 	SlotData.AddUninitialized(NewTab.GetNumSlots());
//
// 	// Initialize each slot's SlotHandle
// 	for (int32 SlotIndex = 0; SlotIndex < NewTab.GetNumSlots(); ++SlotIndex)
// 	{
// 		const int32 AbsoluteSlotIndex = NewTab.GetFirstSlotIndex() + SlotIndex;
// 		checkf(0 <= AbsoluteSlotIndex && AbsoluteSlotIndex < SlotData.Num(),
// 			TEXT("Absolute slot index out of range:"));
//
// 		const FRockInventorySlotHandle SlotHandle(TabIndex, AbsoluteSlotIndex);
// 		SlotData[AbsoluteSlotIndex].SlotHandle = SlotHandle;
// 	}
// 	SlotData.MarkArrayDirty();
//
// 	return TabIndex;
// }


void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, Owner);
	DOREPLIFETIME(URockInventory, ItemData);
	DOREPLIFETIME(URockInventory, SlotData);
	DOREPLIFETIME(URockInventory, SlotSections);
	DOREPLIFETIME(URockInventory, PendingSlotOperations);
}

bool URockInventory::IsSupportedForNetworking() const
{
	return true;
}

#if UE_WITH_IRIS
void URockInventory::RegisterReplicationFragments(
	UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS

void URockInventory::RegisterReplicationWithOwner()
{
	UObject* topLevelOwner = URockInventoryLibrary::GetTopLevelOwner(this);
	if (UActorComponent* Component = Cast<UActorComponent>(topLevelOwner))
	{
		Component->AddReplicatedSubObject(this);
	}
	else if (AActor* actor = Cast<AActor>(topLevelOwner))
	{
		actor->AddReplicatedSubObject(this);
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterReplicationWithOwner - No viable replication owner found"));
	}

	// Iterate over any existing items and register them.
	for (const FRockItemStack& Item : ItemData)
	{
		if (Item.RuntimeInstance)
		{
			Item.RuntimeInstance->RegisterReplicationWithOwner();
		}
	}
}

void URockInventory::UnregisterReplicationWithOwner()
{
	UObject* topLevelOwner = URockInventoryLibrary::GetTopLevelOwner(this);
	if (UActorComponent* Component = Cast<UActorComponent>(topLevelOwner))
	{
		Component->RemoveReplicatedSubObject(this);
	}
	else if (AActor* actor = Cast<AActor>(topLevelOwner))
	{
		actor->RemoveReplicatedSubObject(this);
	}

	// Iterate over items and unregister them?
	for (const FRockItemStack& Item : ItemData)
	{
		if (Item.RuntimeInstance)
		{
			Item.RuntimeInstance->UnregisterReplicationWithOwner();
		}
	}
}

void URockInventory::BroadcastSlotChanged(const FRockSlotDelta& SlotDelta)
{
	OnSlotChanged.Broadcast(SlotDelta);
}

void URockInventory::BroadcastItemChanged(const FRockItemStackHandle& ItemStackHandle, ERockItemChangeType ChangeType)
{
	OnItemChanged.Broadcast(FRockItemDelta(this, ItemStackHandle));
}

void URockInventory::RegisterSlotStatus(AController* Instigator, const FRockInventorySlotHandle& InSlotHandle, ERockSlotStatus InStatus)
{
	// Check if the slot is valid
	if (!InSlotHandle.IsValid())
	{
		return;
	}

	TArray<int32> PendingExpiredIndices;
	constexpr float SlotReservationExpiration = 30.0f;
	const double currentTime = FPlatformTime::Seconds();
	bool bPendingSlotChanged = false;
	bool bIsClaimed = false;
	for (int32 i = 0; i < PendingSlotOperations.Num(); i++)
	{
		const FRockPendingSlotOperation& PendingSlot = PendingSlotOperations[i];

		// This is unrelated to Register, but if we are iterating the array anyway, and this is running on the server
		// we can check if the slot is expired.
		if (PendingSlot.TimeStarted + SlotReservationExpiration < currentTime)
		{
			// This slot is expired, so we can remove it
			PendingExpiredIndices.Add(i);
			// Don't check expired slot for claims
			continue;
		}

		if (PendingSlot.SlotHandle == InSlotHandle)
		{
			if (!FRockPendingSlotOperation::CanClaimSlot(PendingSlot))
			{
				bIsClaimed = true;
				break;
			}
		}
	}
	if (!bIsClaimed)
	{
		// If we reached here, it the slot is claimable.
		FRockPendingSlotOperation NewSlotOperation;
		NewSlotOperation.Controller = Instigator;
		NewSlotOperation.SlotHandle = InSlotHandle;
		// if there is an item at this slot, claim it
		NewSlotOperation.ItemHandle = GetSlotByHandle(InSlotHandle).ItemHandle;
		NewSlotOperation.SlotStatus = InStatus;
		NewSlotOperation.TimeStarted = currentTime;
		PendingSlotOperations.Add(NewSlotOperation);
		bPendingSlotChanged = true;
		// The client-server won't get this call, so we need to call it manually for the listen-server
		// The client receives an onRep from the modification of the above PendingSlotOperation.
	}

	// Remove expired claims from the array
	// Remove from the end to avoid invalidating indices
	PendingExpiredIndices.Sort([](int32 A, int32 B) { return B < A; });
	for (const int32 ExpiredIndex : PendingExpiredIndices)
	{
		// We don't care about the order, so do the more efficient swap remove
		PendingSlotOperations.RemoveAtSwap(ExpiredIndex);
		bPendingSlotChanged = true;
	}


	//if (bPendingSlotChanged)
	{
		OnRep_PendingSlotOperations();
	}
}

void URockInventory::ReleaseSlotStatus(AController* Instigator, const FRockInventorySlotHandle& InSlotHandle)
{
	// Check if the slot is valid
	if (!InSlotHandle.IsValid())
	{
		return;
	}

	for (int32 i = PendingSlotOperations.Num() - 1; i >= 0; i--)
	{
		const FRockPendingSlotOperation& PendingSlot = PendingSlotOperations[i];
		if (PendingSlot.SlotHandle == InSlotHandle && PendingSlot.Controller == Instigator)
		{
			// We found the slot, so we can remove it
			PendingSlotOperations.RemoveAt(i);
			break;
		}
	}

	// The client-server won't get this OnRep call, so we need to call it manually.
	// The client gets it when the PendingSlotOperation gets replicated
	// TODO
	//const AActor* OwningActor = GetOwningActor();
	//if (OwningActor && OwningActor->HasAuthority())
	{
		OnRep_PendingSlotOperations();
	}
}

void URockInventory::OnRep_PendingSlotOperations()
{
	// 1. Build a set of old handles for efficient lookup and removal.
	TSet<FRockInventorySlotHandle> OldHandles;
	// Reserve memory if PreviousPendingSlotOperations can be large to avoid reallocations.
	OldHandles.Reserve(PreviousPendingSlotOperations.Num());
	for (const FRockPendingSlotOperation& OldSlot : PreviousPendingSlotOperations)
	{
		OldHandles.Add(OldSlot.SlotHandle);
	}

	// 2. Iterate through the current slots.
	for (const FRockPendingSlotOperation& NewSlot : PendingSlotOperations)
	{
		// Try to remove the handle from the set of old handles.
		// If Remove returns false, the handle wasn't in the old set, meaning it's newly added.
		if (!OldHandles.Remove(NewSlot.SlotHandle))
		{
			// This is a new pending slot, broadcast its addition.
			// TODO: Perhaps we should leverage a secondary channel for 'pending' changes instead of overloading the existing change type?
			//BroadcastSlotChanged(NewSlot.SlotHandle, ERockSlotChangeType::PendingChange);
		}
		// If Remove returns true, the handle was in the old set and is still present.
		// It has now been removed from OldHandles, so it won't be flagged as removed later.
	}

	// 3. Any handles remaining in OldHandles were present before but are not in the current list.
	// These are the removed slots.
	for (const FRockInventorySlotHandle& RemovedHandle : OldHandles)
	{
		//BroadcastSlotChanged(RemovedHandle, ERockSlotChangeType::PendingChange);
	}

	// 4. Update the previous state for the next comparison.
	PreviousPendingSlotOperations = PendingSlotOperations;
}

ERockSlotStatus URockInventory::GetSlotStatus(const FRockInventorySlotHandle& InSlotHandle) const
{
	// Check if the slot is valid
	if (!InSlotHandle.IsValid())
	{
		return ERockSlotStatus::Empty;
	}

	for (const FRockPendingSlotOperation& PendingSlot : PendingSlotOperations)
	{
		if (PendingSlot.SlotHandle == InSlotHandle)
		{
			return PendingSlot.SlotStatus;
		}
	}
	return ERockSlotStatus::Empty;
}

FRockPendingSlotOperation URockInventory::GetPendingSlotState(const FRockInventorySlotHandle& InSlotHandle) const
{
	// Check if the slot is valid
	if (!InSlotHandle.IsValid())
	{
		return FRockPendingSlotOperation();
	}

	for (const FRockPendingSlotOperation& PendingSlot : PendingSlotOperations)
	{
		if (PendingSlot.SlotHandle == InSlotHandle)
		{
			return PendingSlot;
		}
	}
	return FRockPendingSlotOperation();
}

FString URockInventory::GetDebugString() const
{
	// Is there a better 'name' for this inventory?
	return GetName();
}


uint32 URockInventory::AcquireAvailableItemIndex()
{
	if (FreeIndices.Num() > 0)
	{
		// The item should already have its handle and generation set
		return FreeIndices.Pop(EAllowShrinking::No);
	}
	else if (ItemData.Num() <= SlotData.Num())
	{
		// Generate a new item index
		// Generation gets incremented during the 'release' of an item.
		// Should we grab in 'chunks' like perhaps 5 slots at a time? Though that could cause extra premature replications?
		// though we won't have to trigger array dirty as much?
		// Note: We specifically don't use AddDefaulted_GetRef because we want the Index to be set in the ItemHandle
		const uint32 Index = ItemData.AddDefaulted();
		ItemData[Index].Generation = 0;
		ItemData[Index].ItemHandle = FRockItemStackHandle::Create(Index, 0);
		// Since we modified the element+array, the caller should mark it dirty.
		return Index;
	}
	checkf(false, TEXT("AcquireAvailableItemData - No space left. Inventory is full or not initialized properly."));
	return INDEX_NONE;
}

int32 URockInventory::GetItemStackCount()
{
	int32 Count = 0;
	for (const FRockItemStack& Item : ItemData)
	{
		if (Item.IsValid())
		{
			Count += Item.GetStackCount();
		}
	}
	return Count;
}

int32 URockInventory::GetItemTotalCount()
{
	int32 Count = 0;
	for (const FRockItemStack& Item : ItemData)
	{
		if (Item.IsValid())
		{
			Count += 1; // Count each valid item stack
		}
	}
	return Count;
}

bool URockInventory::IsHandleValid(FRockItemStackHandle ItemHandle) const
{
	if (!ItemHandle.IsValid())
	{
		return false;
	}
	const int32 index = ItemHandle.GetIndex();
	if (!ItemData.ContainsIndex(index))
	{
		return false;
	}
	return ItemData[index].Generation == ItemHandle.GetGeneration();
}

FRockItemReference URockInventory::MakeItemReference(FRockItemStackHandle ItemHandle)
{
	return FRockItemReference(this, ItemHandle);
}

FRockSlotReference URockInventory::MakeSlotReference(FRockInventorySlotHandle SlotHandle)
{
	return FRockSlotReference(this, SlotHandle);
}

FRockItemStackHandle URockInventory::AddItemToInventory(const FRockItemStack& InItemStack)
{
	// We shouldn't have items without a definition
	checkf(InItemStack.IsValid(), TEXT("AddItemToInventory - Invalid item stack"));
	checkf(InItemStack.Definition, TEXT("AddItemToInventory - Invalid item definition"));

	// Let's make sure we are owned by an actor with authority
	AActor* OwningActor = GetOwningActor();
	checkf(OwningActor && OwningActor->HasAuthority(),
	       TEXT("AddItemToInventory - Inventory must be owned by an actor with authority"));

	const int32 PreviousItemDataNum = ItemData.Num();
	const uint32 Index = AcquireAvailableItemIndex();
	checkf(Index != INDEX_NONE, TEXT("AddItemToInventory - Failed to acquire item index"));

	// Modify a reference
	FRockItemStack& NewItemStack = ItemData[Index];
	// Generation is reconciled in the AcquireAvailableItemData
	NewItemStack.Definition = InItemStack.Definition;
	NewItemStack.RuntimeInstance = InItemStack.RuntimeInstance;
	NewItemStack.StackCount = InItemStack.StackCount;
	NewItemStack.CustomValue1 = InItemStack.CustomValue1;
	NewItemStack.CustomValue2 = InItemStack.CustomValue2;

	// Initialize the item stack
	if (NewItemStack.RuntimeInstance != nullptr)
	{
		NewItemStack.RuntimeInstance->Rename(nullptr, this);
		// TODO:
		// Set a Replicated Owner, so clients can call Rename
		// Since the NewOuter isn't replicated by default?
	}
	if (NewItemStack.RuntimeInstance)
	{
		NewItemStack.RuntimeInstance->SetOwningInventory(this);
	}
	if (!NewItemStack.bInitialized)
	{
		NewItemStack.bInitialized = true;

		// TODO: Would we want to do this for 'splits' and not necessarily only once?
		// Right now I think we might disable splits for anything with bRequiresRuntimeInstance
		if (NewItemStack.GetDefinition()->bRequiresRuntimeInstance)
		{
			TSoftClassPtr<class URockItemInstance> RuntimeInstanceClass = NewItemStack.GetDefinition()->RuntimeInstanceClass;
			if (RuntimeInstanceClass.IsValid())
			{
				// TODO: This will synchronously load the class if it isn't already, which could cause hitches. 
				// We should consider preloading or some other strategy if that becomes an issue.
				// At the moment we have no BP RuntimeInstances so this is purely theoretical.
				// As there is nothing to load for C++ defined RuntimeInstances, this is purely a BP concern.
				NewItemStack.RuntimeInstance = NewObject<URockItemInstance>(this, RuntimeInstanceClass.Get());
			}
			else
			{
				NewItemStack.RuntimeInstance = NewObject<URockItemInstance>(this);
			}
			NewItemStack.RuntimeInstance->SetDefinition(NewItemStack.Definition);
		}
		for (const FInstancedStruct& fragment : NewItemStack.GetDefinition()->GetAllFragments())
		{
			const FRockItemFragment* itemFragment = fragment.GetPtr<FRockItemFragment>();
			if (itemFragment)
			{
				itemFragment->OnItemCreated(NewItemStack);
			}
		}
	}

	// Set up the item
	ItemData.MarkItemDirty(ItemData[Index]);
	if (ItemData.Num() != PreviousItemDataNum)
	{
		// Our array changed size. Mark dirty.
		ItemData.MarkArrayDirty();
	}
	BroadcastItemChanged(ItemData[Index].ItemHandle, ERockItemChangeType::Added);
	// Return handle with current index and generation
	return NewItemStack.ItemHandle;
}

void URockInventory::RemoveItemFromInventory(const FRockItemStackHandle& InItemStackHandle)
{
	const int32 InIndex = InItemStackHandle.GetIndex();

	if (!(InIndex != INDEX_NONE && ItemData.ContainsIndex(InIndex)))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ReleaseItemIndex - Invalid index"));
		return;
	}

	if (InItemStackHandle != ItemData[InIndex].ItemHandle)
	{
		// This is an outdated item handle, so we can't remove it
		UE_LOG(LogRockInventory, Warning, TEXT("RemoveItemFromInventory - Invalid item handle"));
		return;
	}

	if (ItemData[InIndex].RuntimeInstance)
	{
		ItemData[InIndex].RuntimeInstance->UnregisterReplicationWithOwner();
	}
	const FRockItemStackHandle OldHandle = ItemData[InIndex].ItemHandle;
	FreeIndices.Add(InIndex);
	// Update the ItemHandle with new Generation
	ItemData[InIndex].Generation++;
	ItemData[InIndex].ItemHandle = FRockItemStackHandle::Create(InIndex, ItemData[InIndex].Generation);
	ItemData[InIndex].Reset();

	// It's common that Remove from FastArray typically would call MarkArrayDirty.
	// But we are not removing the item from the array, just resetting it to be reused later. 
	ItemData.MarkItemDirty(ItemData[InIndex]);

	// We need to broadcast the old handle so that the client can remove it from their inventory.
	BroadcastItemChanged(OldHandle, ERockItemChangeType::Removed);
	// Anything caring about the 'new handle' should be notified by AddItemToInventory
}


void URockInventory::RemoveItemFromInventory(const FRockItemStack& InItemStack)
{
	if (!InItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RemoveItemFromInventory - Invalid item stack"));
		return;
	}
	RemoveItemFromInventory(InItemStack.ItemHandle);
}

void URockInventory::SetItemStackCount(const FRockItemStackHandle& Handle, int32 NewCount, bool bAutoRemoveIfZero)
{
	FRockItemStack Stack = GetItemByHandle(Handle);
	if (NewCount <= 0 && bAutoRemoveIfZero)
	{
		RemoveItemFromInventory(Handle);
		return;
	}
	else
	{
		Stack.StackCount = NewCount;
		SetItemByHandle(Handle, Stack);
	}
}

bool URockInventory::SetItemCustomValueByTag(const FRockItemStackHandle& Handle, FGameplayTag tag, int32 NewCount)
{
	FRockItemStack Stack = GetItemByHandle(Handle);
	if (tag == Stack.GetDefinition()->CustomValue1Tag)
	{
		Stack.CustomValue1 = NewCount;
		SetItemByHandle(Handle, Stack);
		return true;
	}
	else if (tag == Stack.GetDefinition()->CustomValue2Tag)
	{
		Stack.CustomValue2 = NewCount;
		SetItemByHandle(Handle, Stack);
		return true;
	}
	return false;
}

void URockInventory::ForEachSlot(const FRockInventoryQuery& Query, const TFunctionRef<bool(const FRockInventorySectionInfo*, const FRockInventorySlotEntry*)>& Visitor)
{
	for (const FRockInventorySectionInfo& Section : SlotSections)
	{
		if (Query.SectionPredicate && !Query.SectionPredicate(&Section))
		{
			continue;
		}
		const int32 FirstSlotIndex = Section.GetFirstSlotIndex();
		const int32 NumSlots = Section.GetNumSlots();
		for (int32 slotIndex = 0; slotIndex < NumSlots; ++slotIndex)
		{
			const int32 AbsoluteIndex = FirstSlotIndex + slotIndex;
			ensure(AbsoluteIndex < SlotData.Num());
			const FRockInventorySlotEntry* Slot = &SlotData[AbsoluteIndex];

			if (Query.SlotPredicate && !Query.SlotPredicate(Slot))
			{
				continue;
			}

			if (Query.ItemPredicate)
			{
				const FRockItemStack* Stack = GetItemByHandlePtr(Slot->ItemHandle);
				if (!Stack || !Stack->IsValid() || !Query.ItemPredicate(Stack))
					continue;
			}
			if (!Visitor(&Section, Slot))
			{
				// Visitor can return false to break the loop early if needed
				return;
			}
		}
	}
}

TArray<FRockItemStackHandle> URockInventory::FindAllItemHandles(const FRockInventoryQuery& Query)
{
	TArray<FRockItemStackHandle> ResultArr;
	ForEachSlot(Query,
	            [&ResultArr](const FRockInventorySectionInfo* Section, const FRockInventorySlotEntry* Slot)
	            {
		            ResultArr.Add(Slot->ItemHandle);
		            // Continue iterating through all slots
		            return true;
	            });
	return ResultArr;
}

const FRockInventorySlotEntry* URockInventory::FindFirstSlot(const FRockInventoryQuery& Query)
{
	const FRockInventorySlotEntry* Found = nullptr;
	ForEachSlot(Query,
	            [&](const FRockInventorySectionInfo* Section, const FRockInventorySlotEntry* Slot)
	            {
		            if (!Found)
		            {
			            // Note: We need a non-const pointer if we want to modify the slot
			            Found = Slot;
			            return false;
		            }
		            return true;
	            });
	return Found;
}

TArray<FRockInventorySlotEntry> URockInventory::FindAllSlots(const FRockInventoryQuery& Query)
{
	TArray<FRockInventorySlotEntry> ResultArr;
	ForEachSlot(Query,
	            [&ResultArr](const FRockInventorySectionInfo* Section, const FRockInventorySlotEntry* Slot)
	            {
		            ResultArr.Add(*Slot);

		            // Continue iterating through all slots
		            return true;
	            });
	return ResultArr;
}
