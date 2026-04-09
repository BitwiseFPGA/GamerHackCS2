#pragma once
#include <cstdint>

#include "soid.h"
#include "../datatypes/utlvector.h"

// ---------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------
class CSharedObject;

// ---------------------------------------------------------------
// ISharedObjectListener — listener for GC shared object events
// ---------------------------------------------------------------
class ISharedObjectListener
{
public:
	virtual void SOCreated(GCSDK::SOID_t owner, const CSharedObject* pObject, GCSDK::ESOCacheEvent eEvent) = 0;
	virtual void SOUpdated(GCSDK::SOID_t owner, const CSharedObject* pObject, GCSDK::ESOCacheEvent eEvent) = 0;
	virtual void SODestroyed(GCSDK::SOID_t owner, const CSharedObject* pObject, GCSDK::ESOCacheEvent eEvent) = 0;
};

// ---------------------------------------------------------------
// CGCClientSharedObjectTypeCache — cache for objects of a single type
// ---------------------------------------------------------------
class CGCClientSharedObjectTypeCache
{
public:
	/// add an object to this type cache (VFunc index 1)
	bool AddObject(CSharedObject* pObject);

	/// remove an object from this type cache (VFunc index 3)
	bool RemoveObject(CSharedObject* pObject);

	/// get the internal vector of shared objects
	template<typename T>
	[[nodiscard]] CUtlVector<T>& GetVecObjects()
	{
		return *reinterpret_cast<CUtlVector<T>*>(
			reinterpret_cast<std::uintptr_t>(this) + 0x8);
	}
};

// ---------------------------------------------------------------
// CGCClientSharedObjectCache — top-level GC client object cache
// ---------------------------------------------------------------
class CGCClientSharedObjectCache
{
public:
	/// create a base type cache for the given class ID
	CGCClientSharedObjectTypeCache* CreateBaseTypeCache(int nClassID);

	/// find an existing type cache for the given class ID
	CGCClientSharedObjectTypeCache* FindTypeCache(int nClassID);
};
