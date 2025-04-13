// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventorySlot.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "RockInventoryData.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct ROCKINVENTORYRUNTIME_API FRockInventoryData : public FFastArraySerializer
{
	GENERATED_BODY()
private:
	// Force usage of the helpers, and not this array directly. 
	// Replicated list of inventory slots
	UPROPERTY()
	TArray<FRockInventorySlotEntry> AllSlots;
public:

	// FFastArraySerializer implementation
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRockInventorySlotEntry, FRockInventoryData>(AllSlots, DeltaParms, *this);
	}

	// ~Begin of TArray interface
	auto& operator[](int32 Index)
	{
		checkf(0 <= Index && Index < AllSlots.Num(), TEXT("SlotIndex is out of range: %d (Num: %d)"), Index, AllSlots.Num());
		return AllSlots[Index];
	}

	const auto& operator[](int32 Index) const
	{
		checkf(0 <= Index && Index < AllSlots.Num(), TEXT("SlotIndex is out of range: %d (Num: %d)"), Index, AllSlots.Num());
		return AllSlots[Index];
	}

	int32 Num() const { return AllSlots.Num(); }
	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	auto begin() { return AllSlots.begin(); }
	auto begin() const { return AllSlots.begin(); }
	auto end() { return AllSlots.end(); }
	auto end() const { return AllSlots.end(); }
	auto rbegin() { return AllSlots.rbegin(); }
	auto rbegin() const { return AllSlots.rbegin(); }
	auto rend() { return AllSlots.rend(); }
	auto rend() const { return AllSlots.rend(); }
	void Empty() { AllSlots.Empty(); }
	void SetNum(int32 NewNum) { AllSlots.SetNum(NewNum); }
	void AddUninitialized(int32 Count) { AllSlots.AddUninitialized(Count); }
	// ~End of TArray interface
};

template <>
struct TStructOpsTypeTraits<FRockInventoryData> : public TStructOpsTypeTraitsBase2<FRockInventoryData>
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
