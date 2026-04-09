#pragma once

// ---------------------------------------------------------------
// IBaseInterface — root of all Valve interfaces
// ---------------------------------------------------------------
class IBaseInterface
{
public:
	virtual ~IBaseInterface() {}
};

// ---------------------------------------------------------------
// interface creation function types
// ---------------------------------------------------------------
typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
typedef void* (*InstantiateInterfaceFn)();

// ---------------------------------------------------------------
// InterfaceReg — internal interface registration node
// ---------------------------------------------------------------
class InterfaceReg
{
public:
	InstantiateInterfaceFn m_CreateFn;
	const char*            m_pName;
	InterfaceReg*          m_pNext;
};

// ---------------------------------------------------------------
// interface capture helpers
// ---------------------------------------------------------------

/// capture CreateInterface factory from a loaded module
CreateInterfaceFn CaptureFactory(const char* pModuleName);

/// capture a typed interface pointer from a factory
template<typename T>
T* CaptureInterface(CreateInterfaceFn factory, const char* pInterfaceName)
{
	return static_cast<T*>(factory(pInterfaceName, nullptr));
}
