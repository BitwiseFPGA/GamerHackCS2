#pragma once
#include <cstdint>

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../../core/patterns.h"
#include "../datatypes/usercmd.h"
#include "../datatypes/qangle.h"
#include "../functionlist.h"

// CCSGOInput — verified offsets from Andromeda-CS2-Base/CS2/SDK/Update/Offsets.hpp
//   m_bInThirdPerson : 0x229
//   m_pInputMoves    : 0xB58
//   m_vecViewAngles  : pInputMoves + 0x430
class CCSGOInput
{
private:
	std::uint8_t _pad0[PATTERNS::OFFSETS::INPUT_THIRD_PERSON]; // 0x0000

public:
	bool bInThirdPerson; // 0x0229

	void SetViewAngle(QAngle& angView)
	{
		if (SDK_FUNC::SetViewAngles)
			SDK_FUNC::SetViewAngles(this, 0, angView);
	}

	QAngle GetViewAngles()
	{
		if (SDK_FUNC::GetViewAngles)
		{
			QAngle* pAngles = SDK_FUNC::GetViewAngles(this, 0);
			if (pAngles)
				return *pAngles;
		}
		return {};
	}
};
