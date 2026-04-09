#include "config.h"
#include <ShlObj.h>
#include "../utilities/xorstr.h"

// ============================================================================
// JSON helpers (minimal manual JSON — avoids nlohmann dependency)
// ============================================================================

// write a single key-value pair to a stream
static void WriteJsonBool(std::ofstream& f, const char* key, bool val, bool comma = true)
{
	f << "  \"" << key << "\": " << (val ? "true" : "false");
	if (comma) f << ",";
	f << "\n";
}

static void WriteJsonInt(std::ofstream& f, const char* key, int val, bool comma = true)
{
	f << "  \"" << key << "\": " << val;
	if (comma) f << ",";
	f << "\n";
}

static void WriteJsonFloat(std::ofstream& f, const char* key, float val, bool comma = true)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%.6f", val);
	f << "  \"" << key << "\": " << buf;
	if (comma) f << ",";
	f << "\n";
}

static void WriteJsonColor(std::ofstream& f, const char* key, const Color& col, bool comma = true)
{
	f << "  \"" << key << "\": [" << static_cast<int>(col.r) << ", "
		<< static_cast<int>(col.g) << ", " << static_cast<int>(col.b) << ", "
		<< static_cast<int>(col.a) << "]";
	if (comma) f << ",";
	f << "\n";
}

// ============================================================================
// simple JSON parser helpers
// ============================================================================

static void SkipWhitespace(const char*& p)
{
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
}

static bool MatchChar(const char*& p, char c)
{
	SkipWhitespace(p);
	if (*p == c) { ++p; return true; }
	return false;
}

static std::string ParseString(const char*& p)
{
	SkipWhitespace(p);
	if (*p != '"') return {};
	++p;
	std::string result;
	while (*p && *p != '"')
	{
		if (*p == '\\' && *(p + 1))
		{
			++p;
			result += *p;
		}
		else
		{
			result += *p;
		}
		++p;
	}
	if (*p == '"') ++p;
	return result;
}

static double ParseNumber(const char*& p)
{
	SkipWhitespace(p);
	char* end = nullptr;
	double val = strtod(p, &end);
	if (end > p) p = end;
	return val;
}

static bool ParseBool(const char*& p)
{
	SkipWhitespace(p);
	if (strncmp(p, "true", 4) == 0) { p += 4; return true; }
	if (strncmp(p, "false", 5) == 0) { p += 5; return false; }
	return false;
}

// parse [r, g, b, a] color array
static Color ParseColorArray(const char*& p)
{
	SkipWhitespace(p);
	Color col;
	if (*p != '[') return col;
	++p;
	col.r = static_cast<std::uint8_t>(ParseNumber(p)); MatchChar(p, ',');
	col.g = static_cast<std::uint8_t>(ParseNumber(p)); MatchChar(p, ',');
	col.b = static_cast<std::uint8_t>(ParseNumber(p)); MatchChar(p, ',');
	col.a = static_cast<std::uint8_t>(ParseNumber(p));
	MatchChar(p, ']');
	return col;
}

// ============================================================================
// config directory
// ============================================================================

std::filesystem::path C::GetConfigDirectory()
{
	// try Documents/FAGHack/
	wchar_t wszDocuments[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, wszDocuments)))
	{
		std::filesystem::path configDir = std::filesystem::path(wszDocuments) / _XS("GamerHack");
		return configDir;
	}

	// fallback: DLL directory/configs/
	wchar_t wszDllPath[MAX_PATH];
	HMODULE hSelf = nullptr;
	::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(&C::GetConfigDirectory), &hSelf);

	if (hSelf)
	{
		::GetModuleFileNameW(hSelf, wszDllPath, MAX_PATH);
		std::filesystem::path dllDir = std::filesystem::path(wszDllPath).parent_path();
		return dllDir / _XS("configs");
	}

	return std::filesystem::current_path() / _XS("GamerHack");
}

// ============================================================================
// lifecycle
// ============================================================================

bool C::Setup()
{
	L_PRINT(LOG_INFO) << _XS("--- config system setup ---");

	// ensure config directory exists
	const auto configDir = GetConfigDirectory();
	try
	{
		if (!std::filesystem::exists(configDir))
		{
			std::filesystem::create_directories(configDir);
			L_PRINT(LOG_INFO) << _XS("created config directory: ") << configDir.string().c_str();
		}
		else
		{
			L_PRINT(LOG_INFO) << _XS("config directory: ") << configDir.string().c_str();
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to create config directory: ") << ex.what();
		return false;
	}

	// load default config if it exists
	const auto defaultPath = configDir / _XS("default.json");
	if (std::filesystem::exists(defaultPath))
	{
		L_PRINT(LOG_INFO) << _XS("loading default config...");
		LoadFile(_XS("default.json"));
	}
	else
	{
		L_PRINT(LOG_INFO) << _XS("no default config found, using defaults");
	}

	L_PRINT(LOG_INFO) << _XS("config system ready (") << mapVariables.size() << _XS(" variables registered)");
	return true;
}

void C::Destroy()
{
	// auto-save current config
	L_PRINT(LOG_INFO) << _XS("auto-saving config...");
	SaveFile(_XS("default.json"));

	L_PRINT(LOG_INFO) << _XS("config system destroyed");
}

// ============================================================================
// file operations
// ============================================================================

std::vector<std::string> C::GetConfigList()
{
	std::vector<std::string> vecConfigs;
	const auto configDir = GetConfigDirectory();

	try
	{
		if (!std::filesystem::exists(configDir))
			return vecConfigs;

		for (const auto& entry : std::filesystem::directory_iterator(configDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == _XS(".json"))
				vecConfigs.push_back(entry.path().stem().string());
		}
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to enumerate configs: ") << ex.what();
	}

	return vecConfigs;
}

bool C::SaveFile(const std::string& szFileName)
{
	const auto configPath = GetConfigDirectory() / szFileName;

	try
	{
		// ensure directory exists
		std::filesystem::create_directories(configPath.parent_path());

		std::ofstream file(configPath);
		if (!file.is_open())
		{
			L_PRINT(LOG_ERROR) << _XS("failed to open config for writing: ") << configPath.string().c_str();
			return false;
		}

		file << "{\n";

		std::size_t nWritten = 0;
		const std::size_t nTotal = mapVariables.size();

		for (auto& [hash, var] : mapVariables)
		{
			const bool bLastEntry = (++nWritten == nTotal);

			// convert hash to string key
			char szKey[16];
			snprintf(szKey, sizeof(szKey), "%u", hash);

			// determine type and write
			try
			{
				if (var.value.type() == typeid(bool))
				{
					WriteJsonBool(file, szKey, std::any_cast<bool>(var.value), !bLastEntry);
				}
				else if (var.value.type() == typeid(int))
				{
					WriteJsonInt(file, szKey, std::any_cast<int>(var.value), !bLastEntry);
				}
				else if (var.value.type() == typeid(float))
				{
					WriteJsonFloat(file, szKey, std::any_cast<float>(var.value), !bLastEntry);
				}
				else if (var.value.type() == typeid(Color))
				{
					WriteJsonColor(file, szKey, std::any_cast<Color>(var.value), !bLastEntry);
				}
				else
				{
					// unsupported type — skip
					if (!bLastEntry)
						file << "  \"" << szKey << "\": null,\n";
					else
						file << "  \"" << szKey << "\": null\n";
				}
			}
			catch (const std::bad_any_cast&)
			{
				// skip on cast failure
			}
		}

		file << "}\n";
		file.close();

		L_PRINT(LOG_INFO) << _XS("config saved: ") << configPath.string().c_str();
		return true;
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to save config: ") << ex.what();
		return false;
	}
}

bool C::LoadFile(const std::string& szFileName)
{
	const auto configPath = GetConfigDirectory() / szFileName;

	try
	{
		if (!std::filesystem::exists(configPath))
		{
			L_PRINT(LOG_WARNING) << _XS("config file not found: ") << configPath.string().c_str();
			return false;
		}

		std::ifstream file(configPath);
		if (!file.is_open())
		{
			L_PRINT(LOG_ERROR) << _XS("failed to open config for reading: ") << configPath.string().c_str();
			return false;
		}

		// read entire file
		std::string szContent((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		file.close();

		const char* p = szContent.c_str();

		// parse JSON object
		if (!MatchChar(p, '{'))
		{
			L_PRINT(LOG_ERROR) << _XS("invalid config file format");
			return false;
		}

		std::size_t nLoaded = 0;

		while (*p && *p != '}')
		{
			// parse key
			std::string szKey = ParseString(p);
			if (szKey.empty())
				break;

			if (!MatchChar(p, ':'))
				break;

			// convert key to hash
			const FNV1A_t nHash = static_cast<FNV1A_t>(strtoul(szKey.c_str(), nullptr, 10));

			// find the variable
			auto it = mapVariables.find(nHash);
			if (it == mapVariables.end())
			{
				// skip unknown variable — advance past the value
				SkipWhitespace(p);
				if (*p == '"') ParseString(p);
				else if (*p == '[') ParseColorArray(p);
				else if (*p == 't' || *p == 'f') ParseBool(p);
				else ParseNumber(p);
				MatchChar(p, ',');
				continue;
			}

			auto& var = it->second;
			SkipWhitespace(p);

			try
			{
				if (var.value.type() == typeid(bool))
				{
					var.value = ParseBool(p);
					++nLoaded;
				}
				else if (var.value.type() == typeid(int))
				{
					var.value = static_cast<int>(ParseNumber(p));
					++nLoaded;
				}
				else if (var.value.type() == typeid(float))
				{
					var.value = static_cast<float>(ParseNumber(p));
					++nLoaded;
				}
				else if (var.value.type() == typeid(Color))
				{
					var.value = ParseColorArray(p);
					++nLoaded;
				}
				else
				{
					// skip unsupported
					if (*p == '"') ParseString(p);
					else ParseNumber(p);
				}
			}
			catch (...)
			{
				L_PRINT(LOG_WARNING) << _XS("failed to parse config value for hash ") << nHash;
			}

			MatchChar(p, ',');
		}

		L_PRINT(LOG_INFO) << _XS("config loaded: ") << configPath.string().c_str()
			<< " (" << nLoaded << " values)";
		return true;
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to load config: ") << ex.what();
		return false;
	}
}

bool C::DeleteFile(const std::string& szFileName)
{
	const auto configPath = GetConfigDirectory() / szFileName;

	try
	{
		if (std::filesystem::remove(configPath))
		{
			L_PRINT(LOG_INFO) << _XS("config deleted: ") << configPath.string().c_str();
			return true;
		}

		L_PRINT(LOG_WARNING) << _XS("config file not found for deletion: ") << configPath.string().c_str();
		return false;
	}
	catch (const std::exception& ex)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to delete config: ") << ex.what();
		return false;
	}
}
