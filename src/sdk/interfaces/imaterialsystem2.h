#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// CMaterial2 — material resource handle
// ---------------------------------------------------------------
class CMaterial2
{
public:
	[[nodiscard]] const char* GetName();
};

// ---------------------------------------------------------------
// CMaterialSystem2 — material system interface
// ---------------------------------------------------------------
class CMaterialSystem2
{
public:
	/// create a material from resource path and VMT key-values
	/// @param ppEmptyMaterial — output material pointer-to-pointer
	/// @param szNewMaterialName — new material resource name
	/// @param pMaterialData — material key-values data
	CMaterial2** CreateMaterial(CMaterial2*** ppEmptyMaterial, const char* szNewMaterialName, void* pMaterialData);
};
