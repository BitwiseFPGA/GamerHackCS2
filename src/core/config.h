#pragma once
#include "../sdk/common.h"
#include <any>
#include <filesystem>
#include <fstream>

// ---------------------------------------------------------------
// variable registration macros
// ---------------------------------------------------------------

/// register a typed config variable with a default value
#define C_ADD_VARIABLE(type, name, defaultValue) \
	inline auto name = C::AddVariable<type>(FNV1A_HASH(#name), defaultValue)

/// register a fixed-size array config variable
#define C_ADD_VARIABLE_ARRAY(type, size, name, defaultValue) \
	inline auto name = C::AddVariableArray<type, size>(FNV1A_HASH(#name), defaultValue)

// ---------------------------------------------------------------
// config namespace
// ---------------------------------------------------------------
namespace C
{
	/* @section: variable storage */
	struct VariableObject_t
	{
		FNV1A_t      nNameHash = 0;
		FNV1A_t      nTypeHash = 0;
		std::any     value;
	};

	/// internal variable map — hash -> variable object
	inline std::unordered_map<FNV1A_t, VariableObject_t> mapVariables;

	/* @section: variable management */

	/// register a variable, returns its name hash for lookup
	template <typename T>
	FNV1A_t AddVariable(FNV1A_t nNameHash, T defaultValue)
	{
		VariableObject_t var;
		var.nNameHash = nNameHash;
		var.nTypeHash = FNV1A::Hash(typeid(T).name());
		var.value     = std::move(defaultValue);
		mapVariables[nNameHash] = std::move(var);
		return nNameHash;
	}

	/// register an array variable (stored as std::array<T, N>)
	template <typename T, std::size_t N>
	FNV1A_t AddVariableArray(FNV1A_t nNameHash, T defaultValue)
	{
		std::array<T, N> arr;
		arr.fill(defaultValue);

		VariableObject_t var;
		var.nNameHash = nNameHash;
		var.nTypeHash = FNV1A::Hash(typeid(std::array<T, N>).name());
		var.value     = std::move(arr);
		mapVariables[nNameHash] = std::move(var);
		return nNameHash;
	}

	/// get variable reference by name hash
	template <typename T>
	T& Get(FNV1A_t nNameHash)
	{
		auto it = mapVariables.find(nNameHash);
		if (it == mapVariables.end())
		{
			// fatal — variable was never registered
			static T dummy{};
			return dummy;
		}

		return std::any_cast<T&>(it->second.value);
	}

	/* @section: file operations */

	/// @returns: config directory path (Documents/GamerHack/ or DLL-relative)
	std::filesystem::path GetConfigDirectory();

	/// get list of config files in the config directory
	std::vector<std::string> GetConfigList();

	/// save all variables to a JSON file
	bool SaveFile(const std::string& szFileName);

	/// load variables from a JSON file
	bool LoadFile(const std::string& szFileName);

	/// delete a config file
	bool DeleteFile(const std::string& szFileName);

	/* @section: lifecycle */
	bool Setup();
	void Destroy();
}
