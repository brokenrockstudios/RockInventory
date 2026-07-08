// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * ACCESS-CONTROL TOKEN: 
 * "Passkey" idiom, sometimes referred to as an Attorney-Client design pattern. 
 * This class has no data and no purpose except to gate who is allowed to call certain 
 * mutator functions on FRockItemStack (e.g. SetStackCount). If you can construct 
 * one of these, you can mutate state normally read-only from the outside. 
 * If you can't, the compiler stops you.
 *
 * WHY THIS EXISTS INSTEAD OF JUST USING `friend` ON FRockItemStack DIRECTLY:
 * `friend` grants total access, every private member, forever, to the whole
 * friended class. That's usually far more power than the friend actually needs.
 * A friended class doesn't just get to call one setter; it can reach in and touch
 * every private field FRockItemStack has, including ones added *later* that were
 * never intended for that friend. The friend list also has to live inside
 * FRockItemStack itself, so every time a new caller needs access, you're back in
 * the core data struct's header adding another line of trust.
 *
 * This key inverts that: FRockItemStack stays completely closed (no friends of its
 * own). Instead, ONE narrow permission "may call this specific mutator" is
 * factored out into this tiny token type. Only the classes friended HERE can
 * construct a key; they can call only functions that require a key as a parameter. 
 * Everything else about FRockItemStack remains genuinely private, even
 * from these "friends."
 *
 * HOW IT WORKS:
 * The constructor is private, so only friends of THIS class (listed above) can
 * make one. It's also `explicit`. And it simply enables specific gated access to
 * functions that require this key.
 *
 * IMPORTANT: INHERITANCE DOES NOT INHERIT ACCESS:
 * Friendship is not inherited and does not propagate through subclasses. If some other 
 * class derives from ARockInventoryWorldItemBase, it does NOT automatically gain the 
 * ability to construct this key just because its parent can. Each class that needs this 
 * capability must be explicitly added to the friend list below. It keeps the set of 
 * "things that can mutate this" fully enumerable by reading this one file, instead of 
 * growing implicitly through whatever inherits from an already-trusted type.
 *
 * TO GRANT ACCESS TO A NEW CALLER: add `friend class/struct YourType;` above.
 * That is the ONLY change needed. The mutator function signatures don't change,
 * and FRockItemStack itself doesn't need to be touched at all.
 * 
 */
class FRockItemStackMutationKey
{
	friend class ARockInventoryWorldItemBase;
	friend struct FRockItemFragment_SetStats;
	
private:
	FRockItemStackMutationKey() = default;
	FRockItemStackMutationKey(const FRockItemStackMutationKey &) = delete;
	FRockItemStackMutationKey& operator=(const FRockItemStackMutationKey&) = delete;
};
