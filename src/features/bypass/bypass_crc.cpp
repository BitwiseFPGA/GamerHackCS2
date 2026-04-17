#include "bypass_crc.h"

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"

std::string F::BYPASS::CRC::SpoofCRC()
{
	static auto fnComputeMoveCrc = reinterpret_cast<const char*(__fastcall*)(void*)>(
		MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B E9 48 8D 35")));

	if (!fnComputeMoveCrc)
		return {};

	(void)fnComputeMoveCrc;
	return {};
}
