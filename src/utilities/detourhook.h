#pragma once
// used: minhook
// @credits: https://github.com/TsudaKageyu/minhook
#include <MinHook.h>
// used: L_PRINT
#include "log.h"
#include "xorstr.h"

template <typename T>
class CBaseHookObject
{
public:
	/// setup hook and replace function
	/// @returns: true if hook has been successfully created, false otherwise
	bool Create(void* pFunction, void* pDetour)
	{
		if (pFunction == nullptr || pDetour == nullptr)
			return false;

		pBaseFn = pFunction;
		pReplaceFn = pDetour;

		if (const MH_STATUS status = MH_CreateHook(pBaseFn, pReplaceFn, &pOriginalFn); status != MH_OK)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to create hook, status: \"") << MH_StatusToString(status) << "\"";
			return false;
		}

		if (!Replace())
			return false;

		return true;
	}

	/// patch memory to jump to our function instead of original
	bool Replace()
	{
		if (pBaseFn == nullptr)
			return false;

		if (bIsHooked)
			return false;

		if (const MH_STATUS status = MH_EnableHook(pBaseFn); status != MH_OK)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to enable hook, status: \"") << MH_StatusToString(status) << "\"";
			return false;
		}

		bIsHooked = true;
		return true;
	}

	/// restore original function call and cleanup hook data
	bool Remove()
	{
		if (!Restore())
			return false;

		if (const MH_STATUS status = MH_RemoveHook(pBaseFn); status != MH_OK)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to remove hook, status: \"") << MH_StatusToString(status) << "\"";
			return false;
		}

		return true;
	}

	/// restore patched memory to original function call
	bool Restore()
	{
		if (!bIsHooked)
			return false;

		if (const MH_STATUS status = MH_DisableHook(pBaseFn); status != MH_OK)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to restore hook, status: \"") << MH_StatusToString(status) << "\"";
			return false;
		}

		bIsHooked = false;
		return true;
	}

	/// @returns: original, unwrapped function that would be called without the hook
	[[nodiscard]] T GetOriginal()
	{
		return reinterpret_cast<T>(pOriginalFn);
	}

	/// @returns: true if hook is applied at the time, false otherwise
	[[nodiscard]] bool IsHooked() const
	{
		return bIsHooked;
	}

private:
	bool bIsHooked = false;
	void* pBaseFn = nullptr;
	void* pReplaceFn = nullptr;
	void* pOriginalFn = nullptr;
};
