#pragma once
#include "../sdk/common.h"
#include <filesystem>

/*
 * CORE INITIALIZATION ORCHESTRATOR
 * manages the startup and shutdown sequence for all subsystems.
 * order matters: logging -> memory -> interfaces -> schema -> config -> input -> draw -> features -> hooks
 */
namespace CORE
{
	/// DLL module handle (set in DllMain)
	inline HMODULE hModule = nullptr;

	/// path to the loaded DLL
	inline std::filesystem::path dllPath;

	/// true once all subsystems have been initialized
	inline bool bInitialized = false;

	/// set to true to trigger graceful unload
	inline bool bShouldUnload = false;

	/* @section: lifecycle */
	/// initialize all subsystems in order
	/// @returns: true if all critical systems initialized successfully
	bool Setup(HMODULE hMod);

	/// shut down all subsystems in reverse order
	void Destroy();
}
