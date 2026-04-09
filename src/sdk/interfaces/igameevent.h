#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------
class CCSPlayerController;

// ---------------------------------------------------------------
// IGameEvent — game event data accessor
// ---------------------------------------------------------------
class IGameEvent
{
public:
	virtual ~IGameEvent() {}

	/// get the event name
	const char* GetName();

	/// get an int64 value by key name
	std::int64_t GetInt64(const char* pszKeyName, std::int64_t nDefault = 0);

	/// get a player controller by key name
	CCSPlayerController* GetPlayerController(const char* pszKeyName);

	/// get a string value by key name
	const char* GetString(const char* pszKeyName, const char* pszDefault = "");

	/// set a string value by key name
	void SetString(const char* pszKeyName, const char* pszValue);

	/// get an int value by key name
	int GetInt(const char* pszKeyName, int nDefault = 0);

	/// get a float value by key name
	float GetFloat(const char* pszKeyName, float flDefault = 0.f);

	/// get a bool value by key name
	bool GetBool(const char* pszKeyName, bool bDefault = false);
};

// ---------------------------------------------------------------
// IGameEventListener2 — listener for game events
// ---------------------------------------------------------------
class IGameEventListener2
{
public:
	virtual ~IGameEventListener2() {}
	virtual void FireGameEvent(IGameEvent* pEvent) = 0;
};

// ---------------------------------------------------------------
// IGameEventManager2 — game event manager
// ---------------------------------------------------------------
class IGameEventManager2
{
public:
	virtual ~IGameEventManager2() {}
	virtual bool AddListener(IGameEventListener2* pListener, const char* pszName, bool bServerSide) = 0;
	virtual bool RemoveListener(IGameEventListener2* pListener) = 0;
};
