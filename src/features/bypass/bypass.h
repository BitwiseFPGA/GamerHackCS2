#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "../../sdk/datatypes/qangle.h"
#include "../../sdk/datatypes/usercmd.h"

class CCSGOInput;
class CUserCmd;

namespace F::BYPASS
{
	bool Setup();
	void Destroy();

	// called before and after CreateMove feature processing
	void PreCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
	void PostCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);

	// view angle manipulation (anti-cheat safe — writes through input history)
	void SetViewAngles(const QAngle& angView, CCSGOInput* pInput, CUserCmd* pCmd,
	                   bool bAddSetViewAngles = true, bool bOnlyInputHistory = false);

	// attack state manipulation with optional subtick injection
	void SetAttack(CUserCmd* pCmd, bool bAddSubTick = false);
	void SetDontAttack(CUserCmd* pCmd, bool bAddSubTick = false);

	// queue a subtick button press/release
	void AddProcessSubTick(uint64_t nButton, bool bPressed);

	// generate a valid CRC for modified protobuf command data
	std::string SpoofCRC();

	// subtick queue entry
	struct SubTickEntry
	{
		uint64_t nButton;
		bool     bPressed;
		float    flWhen;
	};

	// internal state
	inline std::vector<SubTickEntry> vecSubTicks;
}
