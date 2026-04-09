#pragma once
#include <cstdint>

class CGameEntitySystem;

class IGameResourceService
{
private:
	std::uint8_t _pad0[0x58]; // 0x0000
public:
	CGameEntitySystem* pGameEntitySystem; // 0x0058
};
