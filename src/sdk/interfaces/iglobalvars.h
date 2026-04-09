#pragma once
#include <cstdint>

class IGlobalVars
{
public:
	float flRealTime;           // 0x0000
	std::int32_t nFrameCount;  // 0x0004
	float flFrameTime;          // 0x0008
	float flFrameTime2;         // 0x000C
	std::int32_t nMaxClients;  // 0x0010

private:
	std::uint8_t _pad0[0x1C];  // 0x0014
public:
	float flFrameTime3;         // 0x0030
	float flCurrentTime;        // 0x0034
	float flCurrentTime2;       // 0x0038

private:
	std::uint8_t _pad1[0xC];   // 0x003C
public:
	std::int32_t nTickCount;   // 0x0048

	float flIntervalPerTick;    // 0x004C
};
