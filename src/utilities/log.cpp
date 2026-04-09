#include <ctime>
#include <cstdio>
#include <cstring>
#include <ShlObj.h>

#include "log.h"

// ============================================================================
// static handles
// ============================================================================
static HANDLE hConsoleStream = INVALID_HANDLE_VALUE;
static HANDLE hFileStream    = INVALID_HANDLE_VALUE;

// ============================================================================
// lifecycle
// ============================================================================
bool L::AttachConsole(const wchar_t* wszWindowTitle)
{
	if (::AllocConsole() == FALSE)
		return false;

	hConsoleStream = ::CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hConsoleStream == INVALID_HANDLE_VALUE)
		return false;

	if (::SetStdHandle(STD_OUTPUT_HANDLE, hConsoleStream) == FALSE)
		return false;

	// enable UTF-8 output
	::SetConsoleOutputCP(CP_UTF8);

	if (wszWindowTitle != nullptr)
		::SetConsoleTitleW(wszWindowTitle);

	return true;
}

void L::DetachConsole()
{
	if (hConsoleStream != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hConsoleStream);
		hConsoleStream = INVALID_HANDLE_VALUE;
	}

	::FreeConsole();

	if (const HWND hWnd = ::GetConsoleWindow(); hWnd != nullptr)
		::PostMessageW(hWnd, WM_CLOSE, 0U, 0L);
}

bool L::OpenFile(const wchar_t* wszFileName)
{
	wchar_t wszPath[MAX_PATH];

	// save log to Documents\GamerHackCS2\ directory
	if (SUCCEEDED(::SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, wszPath)))
	{
		wcscat_s(wszPath, MAX_PATH, L"\\GamerHackCS2\\");
	}
	else
	{
		// fallback if SHGetFolderPath fails
		wcscpy_s(wszPath, MAX_PATH, L"C:\\GamerHackCS2\\");
	}

	// create directory if it doesn't exist
	::CreateDirectoryW(wszPath, nullptr);

	wcscat_s(wszPath, MAX_PATH, wszFileName);

	hFileStream = ::CreateFileW(wszPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFileStream == INVALID_HANDLE_VALUE)
		return false;

	// UTF-8 BOM
	::WriteFile(hFileStream, "\xEF\xBB\xBF", 3UL, nullptr, nullptr);
	return true;
}

void L::CloseFile()
{
	if (hFileStream != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFileStream);
		hFileStream = INVALID_HANDLE_VALUE;
	}
}

// ============================================================================
// stream implementation
// ============================================================================

void L::Stream_t::WriteRaw(const char* szText, std::size_t nLength)
{
	if (hConsoleStream != INVALID_HANDLE_VALUE)
		::WriteConsoleA(hConsoleStream, szText, static_cast<DWORD>(nLength), nullptr, nullptr);

	if (hFileStream != INVALID_HANDLE_VALUE)
		::WriteFile(hFileStream, szText, static_cast<DWORD>(nLength), nullptr, nullptr);
}

L::Stream_t& L::Stream_t::operator()(ELogLevel nLevel, const char* szFile, int nLine)
{
	// reset formatting flags each message
	nModeFlags = LOG_MODE_NONE;

	// -- newline (skip for very first print) --
	if (!bFirstPrint)
		WriteRaw("\n", 1);
	bFirstPrint = false;

	// -- timestamp --
	const std::time_t tNow = std::time(nullptr);
	std::tm tmLocal;
	localtime_s(&tmLocal, &tNow);

	char szTime[32];
	const int nTimeLen = snprintf(szTime, sizeof(szTime), "[%02d:%02d:%02d] ",
		tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec);

	if (hConsoleStream != INVALID_HANDLE_VALUE)
	{
		::SetConsoleTextAttribute(hConsoleStream, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		::WriteConsoleA(hConsoleStream, szTime, static_cast<DWORD>(nTimeLen), nullptr, nullptr);
	}
	if (hFileStream != INVALID_HANDLE_VALUE)
		::WriteFile(hFileStream, szTime, static_cast<DWORD>(nTimeLen), nullptr, nullptr);

	// -- file:line block (debug only) --
	if (szFile != nullptr)
	{
		// extract just filename from path
		const char* pFileName = szFile;
		for (const char* p = szFile; *p != '\0'; ++p)
		{
			if (*p == '\\' || *p == '/')
				pFileName = p + 1;
		}

		char szFileBlock[256];
		const int nBlockLen = snprintf(szFileBlock, sizeof(szFileBlock), "[%s:%d] ", pFileName, nLine);

		if (hConsoleStream != INVALID_HANDLE_VALUE)
		{
			::SetConsoleTextAttribute(hConsoleStream, FOREGROUND_INTENSITY);
			::WriteConsoleA(hConsoleStream, szFileBlock, static_cast<DWORD>(nBlockLen), nullptr, nullptr);
		}
		if (hFileStream != INVALID_HANDLE_VALUE)
			::WriteFile(hFileStream, szFileBlock, static_cast<DWORD>(nBlockLen), nullptr, nullptr);
	}

	// -- level block --
	const char* szLevelTag = nullptr;
	LogColorFlags_t nLevelColor = LOG_COLOR_DEFAULT;

	switch (nLevel)
	{
	case LOG_INFO:
		szLevelTag = "[info] ";
		nLevelColor = LOG_COLOR_FORE_CYAN;
		break;
	case LOG_WARNING:
		szLevelTag = "[warning] ";
		nLevelColor = LOG_COLOR_FORE_YELLOW | LOG_COLOR_FORE_INTENSITY;
		break;
	case LOG_ERROR:
		szLevelTag = "[error] ";
		nLevelColor = LOG_COLOR_FORE_RED | LOG_COLOR_FORE_INTENSITY;
		break;
	default:
		break;
	}

	if (szLevelTag != nullptr)
	{
		const auto nTagLen = static_cast<DWORD>(strlen(szLevelTag));

		if (hConsoleStream != INVALID_HANDLE_VALUE)
		{
			::SetConsoleTextAttribute(hConsoleStream, static_cast<WORD>(nLevelColor));
			::WriteConsoleA(hConsoleStream, szLevelTag, nTagLen, nullptr, nullptr);
		}
		if (hFileStream != INVALID_HANDLE_VALUE)
			::WriteFile(hFileStream, szLevelTag, nTagLen, nullptr, nullptr);
	}

	// reset to default color for message body
	if (hConsoleStream != INVALID_HANDLE_VALUE)
		::SetConsoleTextAttribute(hConsoleStream, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	return *this;
}

L::Stream_t& L::Stream_t::operator<<(ColorMarker_t marker)
{
	if (hConsoleStream != INVALID_HANDLE_VALUE)
		::SetConsoleTextAttribute(hConsoleStream, static_cast<WORD>(marker.nFlags));
	return *this;
}

L::Stream_t& L::Stream_t::operator<<(ModeMarker_t marker)
{
	nModeFlags |= marker.nFlags;
	return *this;
}

L::Stream_t& L::Stream_t::operator<<(const char* szMessage)
{
	if (szMessage != nullptr)
		WriteRaw(szMessage, strlen(szMessage));
	return *this;
}

L::Stream_t& L::Stream_t::operator<<(const wchar_t* wszMessage)
{
	if (wszMessage == nullptr)
		return *this;

	// convert wide string to UTF-8
	const int nRequired = ::WideCharToMultiByte(CP_UTF8, 0, wszMessage, -1, nullptr, 0, nullptr, nullptr);
	if (nRequired <= 0)
		return *this;

	// use stack buffer for small strings, heap for large
	char szStackBuffer[512];
	char* szBuffer = szStackBuffer;
	bool bHeapAllocated = false;

	if (nRequired > static_cast<int>(sizeof(szStackBuffer)))
	{
		szBuffer = static_cast<char*>(::HeapAlloc(::GetProcessHeap(), 0, nRequired));
		if (szBuffer == nullptr)
			return *this;
		bHeapAllocated = true;
	}

	::WideCharToMultiByte(CP_UTF8, 0, wszMessage, -1, szBuffer, nRequired, nullptr, nullptr);
	WriteRaw(szBuffer, static_cast<std::size_t>(nRequired - 1)); // exclude null terminator

	if (bHeapAllocated)
		::HeapFree(::GetProcessHeap(), 0, szBuffer);

	return *this;
}

L::Stream_t& L::Stream_t::operator<<(bool bValue)
{
	if (nModeFlags & LOG_MODE_BOOL_ALPHA)
		WriteRaw(bValue ? "true" : "false", bValue ? 4 : 5);
	else
		WriteRaw(bValue ? "1" : "0", 1);
	return *this;
}

L::Stream_t& L::Stream_t::operator<<(const void* pValue)
{
	char szBuffer[24];
	const int nLen = snprintf(szBuffer, sizeof(szBuffer), "0x%p", pValue);
	if (nLen > 0)
		WriteRaw(szBuffer, static_cast<std::size_t>(nLen));
	return *this;
}
