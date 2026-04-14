#pragma once
#include "../sdk/common.h"
#include "../utilities/xorstr.h"

// ---------------------------------------------------------------
// forward declarations for game interfaces
// ---------------------------------------------------------------

// schemasystem.dll
class ISchemaSystem;

// inputsystem.dll
class IInputSystem;

// engine2.dll
class IEngineClient;
class IEngineCVar;

// client.dll
class ISource2Client;
class CGameEntitySystem;
class CCSGOInput;

// tier0.dll / engine2.dll
class IGlobalVars;

// materialsystem2.dll
class IGameResourceService;
class CMaterialSystem2;

// rendersystemdx11.dll
class ISwapChainDx11;

// engine2.dll
class CGameTraceManager;
class CPVS;

// localize.dll
class CLocalize;

// filesystem_stdio.dll
class IBaseFileSystem;

// soundsystem.dll
class CSoundOpSystem;

// client.dll
class CEconItemSystem;

/*
 * INTERFACE CAPTURE SYSTEM
 * captures game interface pointers at init via CreateInterface exports
 * and pattern scanning for pointers not exposed through CreateInterface.
 */
namespace I
{
	/* @section: game interfaces */
	// schemasystem.dll — runtime type info / schema system
	inline ISchemaSystem* SchemaSystem = nullptr;

	// inputsystem.dll — raw input handling
	inline IInputSystem* InputSystem = nullptr;

	// engine2.dll — engine client interface
	inline IEngineClient* Engine = nullptr;

	// client.dll — game client interface
	inline ISource2Client* Client = nullptr;

	// engine2.dll — console variable system
	inline IEngineCVar* Cvar = nullptr;

	// engine2.dll — global timing / tick info (pattern scanned)
	inline IGlobalVars* GlobalVars = nullptr;

	// engine2.dll — game resource / entity service
	inline IGameResourceService* GameResourceService = nullptr;

	// client.dll — entity system (pattern scanned from GameResourceService)
	inline CGameEntitySystem* GameEntitySystem = nullptr;

	// client.dll — CSGO input wrapper (pattern scanned)
	inline CCSGOInput* Input = nullptr;

	// rendersystemdx11.dll — swap chain (pattern scanned)
	inline ISwapChainDx11* SwapChain = nullptr;

	// D3D11 — obtained from swap chain
	inline ID3D11Device* Device = nullptr;
	inline ID3D11DeviceContext* DeviceContext = nullptr;

	// engine2.dll — trace manager (pattern scanned)
	inline CGameTraceManager* GameTraceManager = nullptr;

	// engine2.dll / client.dll — view-projection matrix (pattern scanned)
	inline ViewMatrix* ViewMatrixPtr = nullptr;

	// localize.dll — localization system
	inline CLocalize* Localize = nullptr;

	// filesystem_stdio.dll — base file system
	inline IBaseFileSystem* FileSystem = nullptr;

	// materialsystem2.dll — material system
	inline CMaterialSystem2* MaterialSystem = nullptr;

	// client.dll — econ item system (pattern scanned)
	inline CEconItemSystem* EconItemSystem = nullptr;

	// engine2.dll — PVS (Potentially Visible Set) for model occlusion control
	inline CPVS* PVS = nullptr;

	// client.dll — global CUserCmd** array pointer (used for CUserCmd acquisition in CreateMove)
	inline void* FirstCUserCmdArray = nullptr;

	/* @section: helpers */
	/// capture a CreateInterface-exported interface from a game module
	template <typename T>
	T* CaptureInterface(const char* szModuleName, const char* szInterfaceName)
	{
		using CreateInterfaceFn = T * (*)(const char*, int*);
		const auto fnCreateInterface = reinterpret_cast<CreateInterfaceFn>(
			MEM::GetExportAddress(MEM::GetModuleBaseHandle(szModuleName), _XS("CreateInterface")));

		if (!fnCreateInterface)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to find CreateInterface in ") << szModuleName;
			return nullptr;
		}

		T* pInterface = fnCreateInterface(szInterfaceName, nullptr);
		if (!pInterface)
		{
			L_PRINT(LOG_ERROR) << _XS("failed to capture interface ") << szInterfaceName << _XS(" from ") << szModuleName;
			return nullptr;
		}

		L_PRINT(LOG_INFO) << _XS("captured ") << szInterfaceName << _XS(" from ") << szModuleName
			<< _XS(" = ") << static_cast<const void*>(pInterface);
		return pInterface;
	}

	/* @section: lifecycle */
	bool Setup();
	void PostResolve();  // called after SDK_FUNC::Initialize()
	void Destroy();
}
