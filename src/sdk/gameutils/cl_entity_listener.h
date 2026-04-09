#pragma once
#include "../entity.h"
#include "../entity_handle.h"

// ---------------------------------------------------------------
// IEntityListener — observer interface for entity lifecycle events
//
// Implement this interface and register with GAMEUTIL::AddEntityListener
// to receive callbacks when entities are created or deleted.
// ---------------------------------------------------------------
class IEntityListener
{
public:
    virtual ~IEntityListener() = default;
    virtual void OnEntityCreated(C_BaseEntity* pEntity) {}
    virtual void OnEntityDeleted(C_BaseEntity* pEntity) {}
};

// ---------------------------------------------------------------
// GAMEUTIL — entity listener registration
// ---------------------------------------------------------------
namespace GAMEUTIL
{
    // register an entity listener (does not take ownership)
    void AddEntityListener(IEntityListener* pListener);

    // unregister an entity listener
    void RemoveEntityListener(IEntityListener* pListener);

    // called internally when an entity is added to the system
    void NotifyEntityCreated(C_BaseEntity* pEntity);

    // called internally when an entity is removed from the system
    void NotifyEntityDeleted(C_BaseEntity* pEntity);
}
