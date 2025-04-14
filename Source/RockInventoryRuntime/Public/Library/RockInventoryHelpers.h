// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

/**
 * Macro to provide TArray-like interface for a property that needs to be accessed through FFastArraySerializer
 * This is useful when you want to expose TArray functionality while maintaining the underlying FFastArraySerializer structure
 * 
 * @param PropertyName The name of the TArray property to wrap
 */
#define ROCKINVENTORY_FastArraySerializer_TArray_ACCESSORS(PropertyName) \
	/* Const Iterators */ \
	auto begin() const { return PropertyName.begin(); } \
	auto end() const { return PropertyName.end(); } \
	auto rbegin() const { return PropertyName.rbegin(); } \
	auto rend() const { return PropertyName.rend(); } \
	/* Non-const iterators */ \
	auto begin() { return PropertyName.begin(); } \
	auto end() { return PropertyName.end(); } \
	auto rbegin() { return PropertyName.rbegin(); } \
	auto rend() { return PropertyName.rend(); } \
	\
	/* Const Element Access */ \
	const auto& operator[](int32 Index) const { return PropertyName[Index]; } \
	const auto& Last() const { return PropertyName.Last(); } \
	const auto& Top() const { return PropertyName.Top(); } \
	/* Non-const element access */ \
	auto& operator[](int32 Index) { return PropertyName[Index]; } \
	auto& Last() { return PropertyName.Last(); } \
	auto& Top() { return PropertyName.Top(); } \
	\
	/* Capacity */ \
	int32 Num() const { return PropertyName.Num(); } \
	int32 Max() const { return PropertyName.Max(); } \
	bool IsEmpty() const { return PropertyName.IsEmpty(); } \
	/* Capacity Modifiers */ \
	void Empty() { PropertyName.Empty(); } \
	void SetNum(int32 NewNum) { PropertyName.SetNum(NewNum); } \
	void Reserve(int32 NewCapacity) { PropertyName.Reserve(NewCapacity); } \
	void AddUninitialized(int32 Count) { PropertyName.AddUninitialized(Count); } \
	int32 AddDefaulted(int32 Count) { return PropertyName.AddDefaulted(Count); } \
    int32 AddDefaulted() { return PropertyName.AddDefaulted(); }


// No direct int32 access. Always assume we want to access via handle


// checkf(Index >= 0 && Index < PropertyName.Num(), TEXT("Index out of bounds"));


// These are modifiers and operations
// Which likely require more thought, especially because they might need MarkItemDirty() ir MarkArrayDirty();


//
// \
// /* Modifiers */ \
// int32 Add(const typename decltype(PropertyName)::ElementType& Item) { return PropertyName.Add(Item); } \
// int32 AddUnique(const typename decltype(PropertyName)::ElementType& Item) { return PropertyName.AddUnique(Item); } \
// void Remove(const typename decltype(PropertyName)::ElementType& Item) { PropertyName.Remove(Item); } \
// void RemoveAt(int32 Index) { PropertyName.RemoveAt(Index); } \
// void RemoveAt(int32 Index, int32 Count) { PropertyName.RemoveAt(Index, Count); } \
// void RemoveAll(const TFunctionRef<bool(const typename decltype(PropertyName)::ElementType&)>& Predicate) { PropertyName.RemoveAll(Predicate); } \
// \
// /* Operations */ \
// void Sort(const TFunctionRef<bool(const typename decltype(PropertyName)::ElementType&, const typename decltype(PropertyName)::ElementType&)>& Predicate) { PropertyName.Sort(Predicate); } \
// int32 Find(const typename decltype(PropertyName)::ElementType& Item) const { return PropertyName.Find(Item); } \
// int32 FindLast(const typename decltype(PropertyName)::ElementType& Item) const { return PropertyName.FindLast(Item); } \
// bool Contains(const typename decltype(PropertyName)::ElementType& Item) const { return PropertyName.Contains(Item); } \
// void Swap(int32 A, int32 B) { PropertyName.Swap(A, B); }
