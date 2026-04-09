#include "core/core.h"
#include "utilities/xorstr.h"

static DWORD WINAPI InitThread(LPVOID lpParam)
{
	// wait for critical game modules to load
	while (!GetModuleHandleA(_XS("client.dll")) || !GetModuleHandleA(_XS("engine2.dll")))
		Sleep(100);

	// additional delay for game stability
	Sleep(2000);

	if (!CORE::Setup(static_cast<HMODULE>(lpParam)))
	{
		L_PRINT(LOG_ERROR) << _XS("[DLL] initialization failed, unloading...");
		FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
		return 0;
	}

	// main loop — wait for unload signal
	while (!CORE::bShouldUnload)
	{
		if (GetAsyncKeyState(VK_DELETE) & 0x8000)
			CORE::bShouldUnload = true;

		Sleep(100);
	}

	CORE::Destroy();
	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, [[maybe_unused]] LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		CORE::hModule = hModule;

		if (const HANDLE hThread = CreateThread(nullptr, 0, InitThread, hModule, 0, nullptr))
			CloseHandle(hThread);
	}

	return TRUE;
}
