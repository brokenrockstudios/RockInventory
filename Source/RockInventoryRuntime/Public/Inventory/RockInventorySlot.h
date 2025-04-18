// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockItemOrientation.h"
#include "RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Item/RockItemStackHandle.h"
#include "Library/RockInventoryHelpers.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventorySlot.generated.h"

/**
 * Represents a single slot in an inventory grid.
 * 
 * Each slot can contain an item stack and has properties like orientation and lock state.
 * The slot is part of a larger inventory system and maintains its position through a SlotHandle.
 * 
 * This struct is designed for efficient network replication and cache-friendly memory layout.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FRockInventorySlotEntry();
	static FRockInventorySlotEntry& Invalid();

	/** The actual item in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStackHandle ItemHandle;

	/** Handle to identify this slot's position in the inventory */
	UPROPERTY()
	FRockInventorySlotHandle SlotHandle;

	/** The orientation of the item in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERockItemOrientation Orientation = ERockItemOrientation::Horizontal;

	/** Whether this slot is locked (preventing item movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLocked = false;


	/** Resets the slot to its default state */
	void Reset();

	/** Required for FFastArraySerializerItem */
	void PreReplicatedRemove(const struct FRockInventorySlotContainer& InArraySerializer);
	void PostReplicatedAdd(const struct FRockInventorySlotContainer& InArraySerializer);
	void PostReplicatedChange(const struct FRockInventorySlotContainer& InArraySerializer);

	/** Validates the slot's state */
	bool IsValid() const;

	/** Returns true if the slot is empty */
	// bool IsEmpty() const;

	/** Returns true if the slot can accept the given item */
	// bool CanAcceptItem(const FRockItemStack& NewItem) const;
};


/**
 * Container for inventory slots that uses a packed handle format for efficient indexing.
 * The handle contains relative position within a tab, and the container manages the
 * mapping to absolute indices based on tab configuration.
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventorySlotContainer : public FFastArraySerializer
{
	GENERATED_BODY()

private:
	// Force usage of the helpers, and not this array directly. 
	// Replicated list of inventory slots
public:
	UPROPERTY()
	TArray<FRockInventorySlotEntry> AllSlots;

public:
	ROCKINVENTORY_FastArraySerializer_TArray_ACCESSORS(AllSlots);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRockInventorySlotEntry, FRockInventorySlotContainer>(AllSlots, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FRockInventorySlotContainer> : public TStructOpsTypeTraitsBase2<FRockInventorySlotContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,

		// Other possible options?
		// WithZeroConstructor            = false,                         // struct can be constructed as a valid object by filling its memory footprint with zeroes.
		// WithNoInitConstructor          = false,                         // struct has a constructor which takes an EForceInit parameter which will force the constructor to perform initialization, where the default constructor performs 'uninitialization'.
		// WithNoDestructor               = false,                         // struct will not have its destructor called when it is destroyed.
		// WithCopy                       = !TIsPODType<CPPSTRUCT>::Value, // struct can be copied via its copy assignment operator.
		// WithIdenticalViaEquality       = false,                         // struct can be compared via its operator==.  This should be mutually exclusive with WithIdentical.
		// WithIdentical                  = false,                         // struct can be compared via an Identical(const T* Other, uint32 PortFlags) function.  This should be mutually exclusive with WithIdenticalViaEquality.
		// WithExportTextItem             = false,                         // struct has an ExportTextItem function used to serialize its state into a string.
		// WithImportTextItem             = false,                         // struct has an ImportTextItem function used to deserialize a string into an object of that class.
		// WithAddStructReferencedObjects = false,                         // struct has an AddStructReferencedObjects function which allows it to add references to the garbage collector.
		// WithSerializer                 = false,                         // struct has a Serialize function for serializing its state to an FArchive.
		// WithStructuredSerializer       = false,                         // struct has a Serialize function for serializing its state to an FStructuredArchive.
		// WithPostSerialize              = false,                         // struct has a PostSerialize function which is called after it is serialized
		// WithNetSerializer              = false,                         // struct has a NetSerialize function for serializing its state to an FArchive used for network replication.
		// WithNetDeltaSerializer         = false,                         // struct has a NetDeltaSerialize function for serializing differences in state from a previous NetSerialize operation.
		// WithSerializeFromMismatchedTag = false,                         // struct has a SerializeFromMismatchedTag function for converting from other property tags.
		// WithStructuredSerializeFromMismatchedTag = false,               // struct has an FStructuredArchive-based SerializeFromMismatchedTag function for converting from other property tags.
		// WithPostScriptConstruct        = false,                         // struct has a PostScriptConstruct func tion which is called after it is constructed in blueprints
		// WithNetSharedSerialization     = false,                         // struct has a NetSerialize function that does not require the package map to serialize its state.
		// WithGetPreloadDependencies     = false,                         // struct has a GetPreloadDependencies function to return all objects that will be Preload()ed when the struct is serialized at load time.
		// WithPureVirtual                = false,                         // struct has PURE_VIRTUAL functions and cannot be constructed when CHECK_PUREVIRTUALS is true
		// WithFindInnerPropertyInstance  = false,							// struct has a FindInnerPropertyInstance function that can provide an FProperty and data pointer when given a property FName
		// WithCanEditChange			  = false,							// struct has an editor-only CanEditChange function that can conditionally make child properties read-only in the details panel (same idea as UObject::CanEditChange)
		// WithClearOnFinishDestroy		  = false,							// struct should be cleared during owner UObject's FinishDestroy. Clearing calls destructor and initializes again to default value. This is intended for structs which may need to access UObject pointer members during destruction. Referenced objects may already have their FinishDestroy() called. Clearing should ensure that no UObject pointer members are used during the final destruction.
		// WithVisitor					  = false,							// struct has Visit function that allows to visit additional properties
		// WithIntrusiveOptionalSafeForGC = false,							// struct with an intrusive unset state for TOptional certifies that object fields it contains are nulled in the unset state
	};
};
