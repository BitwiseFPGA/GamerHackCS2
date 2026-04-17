#include "core.h"
#include "interfaces.h"
#include "schema.h"
#include "config.h"
#include "../sdk/functionlist.h"
#include "../utilities/xorstr.h"

// ============================================================================
// forward declarations for subsystems initialized later
// these are defined in their respective files — if not yet created,
// provide stub implementations below
// ============================================================================

// input system (src/utilities/input.h)
namespace IPT { bool Setup(); void Destroy(); }
// draw/render system (src/utilities/render.h)
namespace D { bool Setup(); void Destroy(); }
// feature manager (src/features/)
namespace F { bool Setup(); void Destroy(); }
// hook manager (src/core/hooks.h)
namespace H { bool Setup(); void Destroy(); }

// ============================================================================
// setup
// ============================================================================
bool CORE::Setup(HMODULE hMod)
{
	hModule = hMod;

	// resolve DLL path
	{
		wchar_t wszPath[MAX_PATH];
		::GetModuleFileNameW(hModule, wszPath, MAX_PATH);
		dllPath = std::filesystem::path(wszPath);
	}

	// --- 1. logging (must be first) ---
	try
	{
		if (!L::AttachConsole(_XSW(L"GamerHack CS2")))
		{
			// non-fatal — we can still log to file
		}

		if (!L::OpenFile(_XSW(L"gamerhack.log")))
		{
			// non-fatal
		}
	}
	catch (...)
	{
		// logging failure is non-fatal, continue
	}

	L_PRINT(LOG_INFO) << _XS("========================================");
	L_PRINT(LOG_INFO) << " " << _XS("GamerHack") << _XS(" v") << GamerHack::PROJECT_VERSION;
	L_PRINT(LOG_INFO) << _XS(" build: ") << __DATE__ << " " << __TIME__;
	L_PRINT(LOG_INFO) << _XS(" DLL: ") << dllPath.string().c_str();
	L_PRINT(LOG_INFO) << _XS("========================================");

	// --- 2. interfaces ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing interfaces...");
		if (!I::Setup())
		{
			L_PRINT(LOG_ERROR) << _XS("[CORE] interface capture FAILED");
			return false;
		}
		L_PRINT(LOG_INFO) << _XS("[CORE] interfaces OK");
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] interface exception: ") << ex.what();
		return false;
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] interface unknown exception");
		return false;
	}

	// --- 2.5. SDK function pointers ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing SDK function pointers...");
		if (!SDK_FUNC::Initialize())
		{
			// non-fatal — individual features check their required functions
			L_PRINT(LOG_WARNING) << _XS("[CORE] some SDK functions failed to resolve");
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("[CORE] SDK functions OK");
		}

		// resolve FirstCUserCmdArray fallback using SDK_FUNC if pattern scan failed
		I::PostResolve();
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] SDK func exception: ") << ex.what();
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] SDK func unknown exception");
	}

	// --- 3. schema system ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing schema system...");
		if (!SCHEMA::Setup())
		{
			// non-fatal — schema dump may partially fail on some builds
			L_PRINT(LOG_WARNING) << _XS("[CORE] schema setup returned false (partial failure)");
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("[CORE] schema system OK");
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] schema exception: ") << ex.what();
		// non-fatal, continue
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] schema unknown exception");
	}

	// --- 4. config system ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing config system...");
		if (!C::Setup())
		{
			L_PRINT(LOG_WARNING) << _XS("[CORE] config setup failed (using defaults)");
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("[CORE] config system OK");
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] config exception: ") << ex.what();
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] config unknown exception");
	}

	// --- 5. input system ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing input system...");
		if (!IPT::Setup())
		{
			L_PRINT(LOG_WARNING) << _XS("[CORE] input setup failed");
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("[CORE] input system OK");
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] input exception: ") << ex.what();
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] input unknown exception");
	}

	// --- 6. draw/render system ---
	// Render system initializes lazily on first Present hook call (needs D3D device)
	L_PRINT(LOG_INFO) << _XS("[CORE] draw system will initialize on first Present hook call");

	// --- 7. features ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing features...");
		if (!F::Setup())
		{
			L_PRINT(LOG_WARNING) << _XS("[CORE] feature setup failed");
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("[CORE] features OK");
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] feature exception: ") << ex.what();
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] feature unknown exception");
	}

	// --- 8. hooks (LAST to install) ---
	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] initializing hooks...");
		if (!H::Setup())
		{
			L_PRINT(LOG_ERROR) << _XS("[CORE] hook setup FAILED");
			return false;
		}
		L_PRINT(LOG_INFO) << _XS("[CORE] hooks OK");
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] hook exception: ") << ex.what();
		return false;
	}
	catch (...)
	{
		L_PRINT(LOG_ERROR) << _XS("[CORE] hook unknown exception");
		return false;
	}

	// --- done ---
	bInitialized = true;
	L_PRINT(LOG_INFO) << _XS("========================================");
	L_PRINT(LOG_INFO) << _XS(" initialization complete");
	L_PRINT(LOG_INFO) << _XS("  press INSERT for menu, DEL to unload");
	L_PRINT(LOG_INFO) << _XS("========================================");

	return true;
}

// ============================================================================
// destroy (reverse order)
// ============================================================================
void CORE::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[CORE] shutting down...");

	bInitialized = false;

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] removing hooks...");
		H::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during hook cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying features...");
		F::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during feature cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying draw system...");
		D::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during draw cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying input system...");
		IPT::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during input cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying config system...");
		C::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during config cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying schema data...");
		SCHEMA::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during schema cleanup"); }

	try
	{
		L_PRINT(LOG_INFO) << _XS("[CORE] destroying interfaces...");
		I::Destroy();
	}
	catch (...) { L_PRINT(LOG_ERROR) << _XS("[CORE] exception during interface cleanup"); }

	L_PRINT(LOG_INFO) << _XS("[CORE] shutdown complete, goodbye!");

	L::CloseFile();
	L::DetachConsole();
}
