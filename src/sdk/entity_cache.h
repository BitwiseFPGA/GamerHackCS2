#pragma once
#include <cstdint>
#include <vector>
#include <shared_mutex>
#include <functional>

class C_BaseEntity;
class C_CSPlayerPawn;
class C_PlantedC4;

// ---------------------------------------------------------------
// cached entity entry
// ---------------------------------------------------------------
struct CachedEntity_t
{
	C_BaseEntity*     pEntity   = nullptr;
	std::int32_t      nIndex    = -1;
	std::uint8_t      nTeam     = 0;
	std::int32_t      nHealth   = 0;
	bool              bAlive    = false;
	bool              bDormant  = false;

	[[nodiscard]] bool IsValid() const { return pEntity != nullptr && nIndex >= 0; }
};

// ---------------------------------------------------------------
// IEntityCache — interface for cache observers
// ---------------------------------------------------------------
class IEntityCache
{
public:
	virtual ~IEntityCache() = default;
	virtual void OnAdd(C_BaseEntity* pEntity, int nIndex) = 0;
	virtual void OnRemove(C_BaseEntity* pEntity, int nIndex) = 0;
};

// ---------------------------------------------------------------
// CEntityCache — thread-safe entity caching system
//
// Maintains a snapshot of relevant entities (players, C4, etc.)
// to avoid repeated entity system lookups.
// ---------------------------------------------------------------
class CEntityCache
{
public:
	using OnEntityCallback = std::function<void(C_BaseEntity*, int)>;

	static CEntityCache& Get()
	{
		static CEntityCache s_Instance;
		return s_Instance;
	}

	void Clear()
	{
		std::unique_lock lock(m_Mutex);
		m_vecPlayers.clear();
		m_pLocalPawn  = nullptr;
		m_pPlantedC4  = nullptr;
	}

	void Update();

	// --- accessors (read-locked) ---

	[[nodiscard]] const std::vector<CachedEntity_t>& GetPlayers() const
	{
		return m_vecPlayers;
	}

	[[nodiscard]] C_CSPlayerPawn* GetLocalPawn() const { return m_pLocalPawn; }
	[[nodiscard]] C_PlantedC4*   GetPlantedC4() const { return m_pPlantedC4; }

	std::shared_mutex& GetMutex() { return m_Mutex; }

private:
	CEntityCache() = default;

	mutable std::shared_mutex          m_Mutex;
	std::vector<CachedEntity_t>        m_vecPlayers;
	C_CSPlayerPawn*                    m_pLocalPawn = nullptr;
	C_PlantedC4*                       m_pPlantedC4 = nullptr;
};
