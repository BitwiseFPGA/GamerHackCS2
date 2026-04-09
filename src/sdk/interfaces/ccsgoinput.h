#pragma once
#include <cstdint>

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../datatypes/usercmd.h"
#include "../datatypes/qangle.h"

inline constexpr int MULTIPLAYER_BACKUP = 150;

class CCSGOInput
{
private:
	std::uint8_t _pad0[0x250]; // 0x0000
public:
	CUserCmd arrCommands[MULTIPLAYER_BACKUP]; // 0x0250

private:
	std::uint8_t _pad1[0x99];
public:
	bool bInThirdPerson;

private:
	std::uint8_t _pad2[0x6];
public:
	QAngle angThirdPersonAngles;

private:
	std::uint8_t _pad3[0xE];
public:
	std::int32_t nSequenceNumber;

	CUserCmd* GetUserCmd()
	{
		return &arrCommands[nSequenceNumber % MULTIPLAYER_BACKUP];
	}

	void SetViewAngle(QAngle& angView)
	{
		// SET_VIEW_ANGLES: mid-function entry that skips slot validation (safe for slot=0)
		using fnSetViewAngle = std::int64_t(__fastcall*)(void*, std::int32_t, QAngle&);
		static auto oSetViewAngle = reinterpret_cast<fnSetViewAngle>(
			MEM::FindPattern(_XS("client.dll"), _XS("F2 41 0F 10 00 48 63 CA 48 C1 E1 06")));

		if (oSetViewAngle)
			oSetViewAngle(this, 0, std::ref(angView));
	}

	QAngle GetViewAngles()
	{
		using fnGetViewAngles = std::int64_t(__fastcall*)(CCSGOInput*, std::int32_t);
		static auto oGetViewAngles = reinterpret_cast<fnGetViewAngles>(
			MEM::FindPattern(_XS("client.dll"), _XS("4C 8B C1 83 FA ? 74 ? 48 63 C2")));

		if (oGetViewAngles)
			return *reinterpret_cast<QAngle*>(oGetViewAngles(this, 0));

		return {};
	}
};
