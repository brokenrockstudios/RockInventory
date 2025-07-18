// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"

URockInventory::URockInventory(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
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
	SlotSections.Reserve(config->InventoryTabs.Num()); // Reserve space for the inventory data
	for (const FRockInventorySectionInfo& TabInfo : config->InventoryTabs)
	{
		FRockInventorySectionInfo NewTab = TabInfo;
		NewTab.FirstSlotIndex = totalInventorySlots;
		SlotSections.Add(NewTab);
		totalInventorySlots += NewTab.GetNumSlots();
	}
	// ItemData will grow dynamically, and not be preallocated
	SlotData.SetNum(totalInventorySlots);
	if (SlotData.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory slots defined"));
		return;
	}

	for (int32 SectionIndex = 0; SectionIndex < SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& TabInfo = SlotSections[SectionIndex];
		const int32 TabOffset = TabInfo.FirstSlotIndex;

		for (int32 SlotIndex = 0; SlotIndex < TabInfo.GetNumSlots(); ++SlotIndex)
		{
			const int32 AbsoluteSlotIndex = TabOffset + SlotIndex;
			SlotData[AbsoluteSlotIndex].SlotHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteSlotIndex);
			SlotData[AbsoluteSlotIndex].ItemHandle = FRockItemStackHandle::Invalid();
			SlotData[AbsoluteSlotIndex].Orientation = ERockItemOrientation::Horizontal;
			SlotData[AbsoluteSlotIndex].bIsLocked = false;
		}
	}

	SlotData.MarkArrayDirty();
	ItemData.MarkArrayDirty();
}


FRockInventorySectionInfo URockInventory::GetSectionInfo(const FName& SectionName) const
{
	if (SectionName != NAME_None)
	{
		for (const FRockInventorySectionInfo& SectionInfo : SlotSections)
		{
			if (SectionInfo.SectionName == SectionName)
			{
				return SectionInfo;
			}
		}
	}
	return FRockInventorySectionInfo::Invalid();
}

FRockInventorySectionInfo URockInventory::GetSectionInfoBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	if (InSlotHandle.IsValid())
	{
		const int32 SectionIndex = InSlotHandle.GetSectionIndex();
		if (SlotSections.IsValidIndex(SectionIndex))
		{
			return SlotSections[SectionIndex];
		}
	}
	return FRockInventorySectionInfo::Invalid();
}

int32 URockInventory::GetSectionIndexById(const FName& SectionName) const
{
	if (SectionName != NAME_None)
	{
		for (int32 SlotIndex = 0; SlotIndex < SlotSections.Num(); SlotIndex++)
		{
			if (SlotSections[SlotIndex].SectionName == SectionName)
			{
				return SlotIndex;
			}
		}
	}
	return INDEX_NONE;
}


FRockInventorySlotEntry URockInventory::GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotByHandle - Invalid slot index"));
		return FRockInventorySlotEntry::Invalid();
	}
	return SlotData[slotIndex];
}

FRockItemStack URockInventory::GetItemBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const FRockInventorySlotEntry& Slot = GetSlotByHandle(InSlotHandle);
	return GetItemByHandle(Slot.ItemHandle);
}

FRockItemStack URockInventory::GetItemByHandle(const FRockItemStackHandle& InSlotHandle) const
{
	if (!InSlotHandle.IsValid())
	{
		return FRockItemStack::Invalid();
	}
	const int32 index = InSlotHandle.GetIndex();
	if (!ItemData.ContainsIndex(index))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("Bad Slot Index"));
		return FRockItemStack::Invalid();
	}
	FRockItemStack Item = ItemData[index];
	if (Item.Generation == InSlotHandle.GetGeneration())
	{
		return Item;
	}
	UE_LOG(LogRockInventory, Warning, TEXT("Bad Generation %d %d"), Item.Generation, InSlotHandle.GetGeneration());
	return FRockItemStack::Invalid();
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
	// Just doing an assignment operator seemingly was triggering an add/remove. for some reason?
	// Possibly because its non-polymorphic and the base class has some values that we don't want to change?

	ChangedItem.ItemHandle = InItemStack.ItemHandle;
	ChangedItem.Definition = InItemStack.Definition;
	ChangedItem.StackSize = InItemStack.StackSize;
	ChangedItem.CustomValue1 = InItemStack.CustomValue1;
	ChangedItem.CustomValue2 = InItemStack.CustomValue2;
	ChangedItem.RuntimeInstance = InItemStack.RuntimeInstance;
	ChangedItem.Generation = InItemStack.Generation;

	ItemData.MarkItemDirty(ChangedItem);
	BroadcastItemChanged(InSlotHandle);
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlotEntry)
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot index"));
		return;
	}

	FRockInventorySlotEntry& ChangedSlot = SlotData[slotIndex];
	ChangedSlot.ItemHandle = InSlotEntry.ItemHandle;
	ChangedSlot.SlotHandle = InSlotEntry.SlotHandle;
	ChangedSlot.Orientation = InSlotEntry.Orientation;
	ChangedSlot.bIsLocked = InSlotEntry.bIsLocked;
	SlotData.MarkItemDirty(ChangedSlot);

	BroadcastSlotChanged(InSlotHandle);
}

int32 URockInventory::AddSection(const FName& SectionName, int32 Width, int32 Height)
{
	FRockInventorySectionInfo NewTab;
	NewTab.SectionName = SectionName;
	NewTab.Width = Width;
	NewTab.Height = Height;
	NewTab.FirstSlotIndex = SlotData.Num(); // Current size is the first index

	const int32 TabIndex = SlotSections.Add(NewTab);

	// Reserve space for the new tab's slots
	SlotData.AddUninitialized(NewTab.GetNumSlots());

	// Initialize each slot's SlotHandle
	for (int32 SlotIndex = 0; SlotIndex < NewTab.GetNumSlots(); ++SlotIndex)
	{
		const int32 AbsoluteSlotIndex = NewTab.FirstSlotIndex + SlotIndex;
		checkf(0 <= AbsoluteSlotIndex && AbsoluteSlotIndex < SlotData.Num(),
			TEXT("Absolute slot index out of range:"));

		const FRockInventorySlotHandle SlotHandle(TabIndex, AbsoluteSlotIndex);
		SlotData[AbsoluteSlotIndex].SlotHandle = SlotHandle;
	}
	SlotData.MarkArrayDirty();

	return TabIndex;
}


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

void URockInventory::BroadcastSlotChanged(const FRockInventorySlotHandle& SlotHandle)
{
	OnSlotChanged.Broadcast(this, SlotHandle);
}

void URockInventory::BroadcastItemChanged(const FRockItemStackHandle& ItemStackHandle)
{
	OnItemChanged.Broadcast(this, ItemStackHandle);
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
		// if the there is an item at this slot, claim it
		NewSlotOperation.ItemHandle = GetSlotByHandle(InSlotHandle).ItemHandle;
		NewSlotOperation.SlotStatus = InStatus;
		NewSlotOperation.TimeStarted = currentTime;
		PendingSlotOperations.Add(NewSlotOperation);
		bPendingSlotChanged = true;
		// The client-server won't get this call, so we need to call it manually for the listen-server
		// The client receives an onrep from the modification of the above PendingSlotOperation.
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
			// This is a new slot, broadcast its addition.
			BroadcastSlotChanged(NewSlot.SlotHandle);
		}
		// If Remove returns true, the handle was in the old set and is still present.
		// It has now been removed from OldHandles, so it won't be flagged as removed later.
	}

	// 3. Any handles remaining in OldHandles were present before but are not in the current list.
	// These are the removed slots.
	for (const FRockInventorySlotHandle& RemovedHandle : OldHandles)
	{
		BroadcastSlotChanged(RemovedHandle);
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
		// Since we modified the array, we need to mark it dirty? Or can we just mark the item only?
		ItemData.MarkItemDirty(ItemData[Index]);
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
			Count += Item.StackSize;
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


FRockItemStackHandle URockInventory::AddItemToInventory(const FRockItemStack& InItemStack)
{
	// We shouldn't have items without a definition
	checkf(InItemStack.IsValid(), TEXT("AddItemToInventory - Invalid item stack"));
	checkf(InItemStack.Definition, TEXT("AddItemToInventory - Invalid item definition"));

	// Let's make sure we are owned by an actor with authority
	AActor* OwningActor = GetOwningActor();
	checkf(OwningActor && OwningActor->HasAuthority(),
		TEXT("AddItemToInventory - Inventory must be owned by an actor with authority"));


	const uint32 Index = AcquireAvailableItemIndex();
	checkf(Index != INDEX_NONE, TEXT("AddItemToInventory - Failed to acquire item index"));

	// Modify a reference
	FRockItemStack& NewItemStack = ItemData[Index];
	// Generation is reconciled in the AcquireAvailableItemData
	NewItemStack.Definition = InItemStack.Definition;
	NewItemStack.RuntimeInstance = InItemStack.RuntimeInstance;
	NewItemStack.StackSize = InItemStack.StackSize;
	NewItemStack.CustomValue1 = InItemStack.CustomValue1;
	NewItemStack.CustomValue2 = InItemStack.CustomValue2;

	// Initialize the item stack
	if (NewItemStack.GetDefinition()->bRequiresRuntimeInstance)
	{
		if (NewItemStack.RuntimeInstance != nullptr)
		{
			NewItemStack.RuntimeInstance->Rename(nullptr, this);
		}
		else
		{
			NewItemStack.RuntimeInstance = NewObject<URockItemInstance>(this);
			NewItemStack.RuntimeInstance->SetDefinition(NewItemStack.Definition);

			for (auto fragment : NewItemStack.GetDefinition()->Fragments)
			{
				const FRockItemFragment* itemFragment = fragment.GetFragmentData<FRockItemFragment>();

				itemFragment->OnItemCreated(NewItemStack.RuntimeInstance);
			}
		}
		NewItemStack.RuntimeInstance->SetOwningInventory(this);
	}
	// Set up the item
	ItemData.MarkItemDirty(ItemData[Index]);
	BroadcastItemChanged(ItemData[Index].ItemHandle);
	// Return handle with current index and generation
	return NewItemStack.ItemHandle;
}


void URockInventory::RemoveItemFromInventory(const FRockItemStack& InItemStack)
{
	if (!InItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RemoveItemFromInventory - Invalid item stack"));
		return;
	}
	const int32 InIndex = InItemStack.ItemHandle.GetIndex();
	if (InIndex != INDEX_NONE && ItemData.ContainsIndex(InIndex))
	{
		if (InItemStack.ItemHandle != ItemData[InIndex].ItemHandle)
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
		BroadcastItemChanged(OldHandle);
		// Anything caring about the 'new handle' should be notified by AddItemToInventory
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ReleaseItemIndex - Invalid index"));
	}
}
