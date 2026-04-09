#include "cl_entity_listener.h"
#include <vector>
#include <algorithm>
#include <mutex>

// ---------------------------------------------------------------
// internal state
// ---------------------------------------------------------------
static std::mutex s_listenerMutex;
static std::vector<IEntityListener*> s_vecListeners;

// ---------------------------------------------------------------
// AddEntityListener
// ---------------------------------------------------------------
void GAMEUTIL::AddEntityListener(IEntityListener* pListener)
{
    if (!pListener)
        return;

    std::lock_guard<std::mutex> lock(s_listenerMutex);

    // prevent duplicates
    auto it = std::find(s_vecListeners.begin(), s_vecListeners.end(), pListener);
    if (it == s_vecListeners.end())
        s_vecListeners.push_back(pListener);
}

// ---------------------------------------------------------------
// RemoveEntityListener
// ---------------------------------------------------------------
void GAMEUTIL::RemoveEntityListener(IEntityListener* pListener)
{
    if (!pListener)
        return;

    std::lock_guard<std::mutex> lock(s_listenerMutex);
    s_vecListeners.erase(
        std::remove(s_vecListeners.begin(), s_vecListeners.end(), pListener),
        s_vecListeners.end());
}

// ---------------------------------------------------------------
// NotifyEntityCreated
// ---------------------------------------------------------------
void GAMEUTIL::NotifyEntityCreated(C_BaseEntity* pEntity)
{
    if (!pEntity)
        return;

    std::lock_guard<std::mutex> lock(s_listenerMutex);
    for (auto* pListener : s_vecListeners)
    {
        if (pListener)
            pListener->OnEntityCreated(pEntity);
    }
}

// ---------------------------------------------------------------
// NotifyEntityDeleted
// ---------------------------------------------------------------
void GAMEUTIL::NotifyEntityDeleted(C_BaseEntity* pEntity)
{
    if (!pEntity)
        return;

    std::lock_guard<std::mutex> lock(s_listenerMutex);
    for (auto* pListener : s_vecListeners)
    {
        if (pListener)
            pListener->OnEntityDeleted(pEntity);
    }
}
