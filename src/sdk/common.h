#pragma once

// ---------------------------------------------------------------
// project info
// ---------------------------------------------------------------
namespace GamerHack
{
	inline constexpr const char* PROJECT_NAME    = "GamerHack";
	inline constexpr const char* PROJECT_VERSION = "1.0.0";
}

// ---------------------------------------------------------------
// windows headers
// ---------------------------------------------------------------
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

// ---------------------------------------------------------------
// standard library
// ---------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <format>
#include <span>
#include <concepts>
#include <type_traits>
#include <numbers>
#include <bit>
#include <limits>

// ---------------------------------------------------------------
// directx 11
// ---------------------------------------------------------------
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// ---------------------------------------------------------------
// project utilities (conditionally included)
// ---------------------------------------------------------------
#if __has_include("../utilities/log.h")
#include "../utilities/log.h"
#endif

#if __has_include("../utilities/memory.h")
#include "../utilities/memory.h"
#endif

#if __has_include("../utilities/math.h")
#include "../utilities/math.h"
#endif

#if __has_include("../utilities/fnv1a.h")
#include "../utilities/fnv1a.h"
#endif

#if __has_include("../utilities/xorstr.h")
#include "../utilities/xorstr.h"
#endif

// ---------------------------------------------------------------
// SDK data types
// ---------------------------------------------------------------
#include "datatypes/vector.h"
#include "datatypes/vector2.h"
#include "datatypes/vector4.h"
#include "datatypes/qangle.h"
#include "datatypes/matrix.h"
#include "datatypes/color.h"
#include "datatypes/rect.h"
#include "datatypes/utlmemory.h"
#include "datatypes/utlvector.h"
#include "datatypes/utlstring.h"
#include "datatypes/utlmap.h"
#include "datatypes/utlsymbol.h"
#include "datatypes/stronghandle.h"
#include "entity_handle.h"
#include "const.h"

// ---------------------------------------------------------------
// common macros
// ---------------------------------------------------------------
#ifdef _DEBUG
#define FAG_ASSERT(EXPR) static_cast<void>(!!(EXPR) || (__debugbreak(), 0))
#else
#define FAG_ASSERT(EXPR) static_cast<void>(0)
#endif

/// padding macro for matching game struct layouts
#define _PAD_CONCAT_IMPL(a, b) a##b
#define _PAD_CONCAT(a, b) _PAD_CONCAT_IMPL(a, b)
#define PAD(SIZE) \
private: \
	std::uint8_t _PAD_CONCAT(_pad, __COUNTER__)[SIZE]; \
public:

/// compiler force-inline
#ifndef FORCEINLINE
#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __attribute__((always_inline)) inline
#endif
#endif

/// check if a virtual key is currently pressed
#define IS_KEY_DOWN(VK) ((::GetAsyncKeyState(VK) & 0x8000) != 0)

/// prevent copy/assignment for game-mirror classes
#define CS_CLASS_NO_ASSIGNMENT(CLASS_NAME) \
	CLASS_NAME(const CLASS_NAME&) = delete; \
	CLASS_NAME& operator=(const CLASS_NAME&) = delete;

// ---------------------------------------------------------------
// abstract interfaces
// ---------------------------------------------------------------

/// minimal handle-entity interface for entity-system interop
class IHandleEntity
{
public:
	virtual ~IHandleEntity() = default;
};

/// minimal economy item interface
class IEconItemInterface
{
public:
	virtual ~IEconItemInterface() = default;
};
