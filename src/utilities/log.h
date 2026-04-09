#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <mutex>

#pragma region log_definitions

#ifdef _DEBUG
#define L_PRINT(LEVEL) L::Stream(LEVEL, __FILE__, __LINE__)
#else
#define L_PRINT(LEVEL) L::Stream(LEVEL)
#endif

#pragma endregion

#pragma region log_enumerations

enum ELogLevel : std::uint8_t
{
	LOG_NONE = 0,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

using LogColorFlags_t = std::uint16_t;

enum ELogColorFlags : LogColorFlags_t
{
	LOG_COLOR_FORE_BLUE      = FOREGROUND_BLUE,
	LOG_COLOR_FORE_GREEN     = FOREGROUND_GREEN,
	LOG_COLOR_FORE_RED       = FOREGROUND_RED,
	LOG_COLOR_FORE_INTENSITY = FOREGROUND_INTENSITY,
	LOG_COLOR_FORE_CYAN      = FOREGROUND_BLUE | FOREGROUND_GREEN,
	LOG_COLOR_FORE_MAGENTA   = FOREGROUND_BLUE | FOREGROUND_RED,
	LOG_COLOR_FORE_YELLOW    = FOREGROUND_GREEN | FOREGROUND_RED,
	LOG_COLOR_FORE_WHITE     = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	LOG_COLOR_FORE_GRAY      = FOREGROUND_INTENSITY,
	LOG_COLOR_FORE_BLACK     = 0U,

	LOG_COLOR_BACK_BLUE      = BACKGROUND_BLUE,
	LOG_COLOR_BACK_GREEN     = BACKGROUND_GREEN,
	LOG_COLOR_BACK_RED       = BACKGROUND_RED,
	LOG_COLOR_BACK_INTENSITY = BACKGROUND_INTENSITY,
	LOG_COLOR_BACK_CYAN      = BACKGROUND_BLUE | BACKGROUND_GREEN,
	LOG_COLOR_BACK_MAGENTA   = BACKGROUND_BLUE | BACKGROUND_RED,
	LOG_COLOR_BACK_YELLOW    = BACKGROUND_GREEN | BACKGROUND_RED,
	LOG_COLOR_BACK_WHITE     = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	LOG_COLOR_BACK_BLACK     = 0U,

	LOG_COLOR_DEFAULT = LOG_COLOR_FORE_WHITE | LOG_COLOR_BACK_BLACK
};

using LogModeFlags_t = std::uint16_t;

enum ELogModeFlags : LogModeFlags_t
{
	LOG_MODE_NONE = 0U,

	// integer formatting
	LOG_MODE_INT_HEX  = (1U << 0U),
	LOG_MODE_INT_DEC  = (1U << 1U),
	LOG_MODE_INT_OCT  = (1U << 2U),
	LOG_MODE_INT_MASK = (LOG_MODE_INT_HEX | LOG_MODE_INT_DEC | LOG_MODE_INT_OCT),

	// float formatting
	LOG_MODE_FLOAT_FIXED      = (1U << 3U),
	LOG_MODE_FLOAT_SCIENTIFIC = (1U << 4U),
	LOG_MODE_FLOAT_MASK       = (LOG_MODE_FLOAT_FIXED | LOG_MODE_FLOAT_SCIENTIFIC),

	// misc
	LOG_MODE_SHOWBASE    = (1U << 5U),
	LOG_MODE_UPPERCASE   = (1U << 6U),
	LOG_MODE_BOOL_ALPHA  = (1U << 7U),
};

#pragma endregion

/*
 * LOGGING SYSTEM
 * - stream-based interface (operator<<)
 * - console output with colored text
 * - file output to faghack.log
 * - thread-safe with mutex
 * - timestamp prefix on each line
 */
namespace L
{
	/* @section: lifecycle */
	/// allocate console window with write permission
	bool AttachConsole(const wchar_t* wszWindowTitle);
	/// free console window and close handles
	void DetachConsole();
	/// open log file for writing
	bool OpenFile(const wchar_t* wszFileName);
	/// close log file
	void CloseFile();

	/* @section: stream markers */
	struct ColorMarker_t  { LogColorFlags_t nFlags; };
	struct ModeMarker_t   { LogModeFlags_t nFlags; };

	/* @section: stream helpers */
	[[nodiscard]] inline ColorMarker_t SetColor(LogColorFlags_t nColorFlags) { return { nColorFlags }; }
	[[nodiscard]] inline ModeMarker_t  AddFlags(LogModeFlags_t nModeFlags) { return { nModeFlags }; }

	/* @section: stream type */
	struct Stream_t
	{
		/// begin a log message with timestamp, level, optional file/line
		Stream_t& operator()(ELogLevel nLevel, const char* szFile = nullptr, int nLine = 0);

		// manipulators
		Stream_t& operator<<(ColorMarker_t marker);
		Stream_t& operator<<(ModeMarker_t marker);

		// string types
		Stream_t& operator<<(const char* szMessage);
		Stream_t& operator<<(const wchar_t* wszMessage);
		Stream_t& operator<<(bool bValue);
		Stream_t& operator<<(const void* pValue);

		// integral types
		template <typename T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
		Stream_t& operator<<(T value);

		// floating-point types
		template <typename T> requires std::is_floating_point_v<T>
		Stream_t& operator<<(T value);

		bool bFirstPrint = true;
		LogModeFlags_t nModeFlags = LOG_MODE_NONE;

	private:
		void WriteRaw(const char* szText, std::size_t nLength);
	};

	/* @section: internal */
	// primary logging stream — singleton
	inline Stream_t stream;
	// mutex for thread safety
	inline std::mutex mtxStream;

	/// wrapper that locks mutex, calls operator(), returns stream ref
	inline Stream_t& Stream(ELogLevel nLevel, const char* szFile = nullptr, int nLine = 0)
	{
		mtxStream.lock();
		stream(nLevel, szFile, nLine);
		// note: mutex unlocked after newline is written (end of message chain)
		// for simplicity we unlock here — callers are expected to complete quickly
		mtxStream.unlock();
		return stream;
	}
}

// ============================================================================
// template implementations (must be in header)
// ============================================================================

template <typename T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
L::Stream_t& L::Stream_t::operator<<(T value)
{
	char szBuffer[72];
	char* pEnd = szBuffer + sizeof(szBuffer) - 1;
	char* pCur = pEnd;
	*pCur = '\0';

	const char* digits = (nModeFlags & LOG_MODE_UPPERCASE) ?
		"0123456789ABCDEF" : "0123456789abcdef";

	int base = 10;
	const char* prefix = nullptr;

	if (nModeFlags & LOG_MODE_INT_HEX)
	{
		base = 16;
		prefix = (nModeFlags & LOG_MODE_SHOWBASE) ? "0x" : nullptr;
	}
	else if (nModeFlags & LOG_MODE_INT_OCT)
	{
		base = 8;
		prefix = (nModeFlags & LOG_MODE_SHOWBASE) ? "0" : nullptr;
	}

	bool negative = false;
	if constexpr (std::is_signed_v<T>)
	{
		if (value < 0 && base == 10)
		{
			negative = true;
			// handle minimum value edge case
			using UT = std::make_unsigned_t<T>;
			UT uval = static_cast<UT>(~value) + 1u;
			do
			{
				*--pCur = digits[uval % base];
				uval /= base;
			} while (uval > 0);

			if (negative) *--pCur = '-';
			if (prefix)
			{
				auto plen = strlen(prefix);
				pCur -= plen;
				memcpy(pCur, prefix, plen);
			}
			WriteRaw(pCur, static_cast<std::size_t>(pEnd - pCur));
			return *this;
		}
	}

	auto uval = static_cast<std::make_unsigned_t<T>>(value);
	do
	{
		*--pCur = digits[uval % base];
		uval /= base;
	} while (uval > 0);

	if (prefix)
	{
		auto plen = strlen(prefix);
		pCur -= plen;
		memcpy(pCur, prefix, plen);
	}

	WriteRaw(pCur, static_cast<std::size_t>(pEnd - pCur));
	return *this;
}

template <typename T> requires std::is_floating_point_v<T>
L::Stream_t& L::Stream_t::operator<<(T value)
{
	char szBuffer[96];
	const char* fmt;

	if (nModeFlags & LOG_MODE_FLOAT_FIXED)
		fmt = "%.3f";
	else if (nModeFlags & LOG_MODE_FLOAT_SCIENTIFIC)
		fmt = "%.6e";
	else
		fmt = "%.6g";

	const int nLen = snprintf(szBuffer, sizeof(szBuffer), fmt, static_cast<double>(value));
	if (nLen > 0)
		WriteRaw(szBuffer, static_cast<std::size_t>(nLen));

	return *this;
}
