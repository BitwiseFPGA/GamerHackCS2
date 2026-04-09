#pragma once
#include <cstdint>
#include <cstddef>

using FNV1A_t = std::uint32_t;

/*
 * 32-BIT FNV-1a HASH
 * used for: schema lookups, config keys, string comparisons
 *
 * basis: 0x811C9DC5
 * prime: 0x01000193
 */
namespace FNV1A
{
	/* @section: constants */
	inline constexpr FNV1A_t BASIS = 0x811C9DC5u;
	inline constexpr FNV1A_t PRIME = 0x01000193u;

	/* @section: helpers */
	/// convert character to lowercase at compile-time
	[[nodiscard]] constexpr char ToLower(const char c) noexcept
	{
		return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
	}

	[[nodiscard]] constexpr wchar_t ToLowerW(const wchar_t c) noexcept
	{
		return (c >= L'A' && c <= L'Z') ? (c + (L'a' - L'A')) : c;
	}

	/* @section: compile-time hashing */
	/// compile-time hash for narrow strings (case-sensitive)
	/// @returns: FNV-1a hash computed at compile-time
	[[nodiscard]] consteval FNV1A_t HashConst(const char* szString, const FNV1A_t uHash = BASIS) noexcept
	{
		return (szString[0] == '\0') ? uHash : HashConst(&szString[1], (uHash ^ static_cast<FNV1A_t>(szString[0])) * PRIME);
	}

	/// compile-time hash for narrow strings (case-insensitive)
	[[nodiscard]] consteval FNV1A_t HashConstI(const char* szString, const FNV1A_t uHash = BASIS) noexcept
	{
		return (szString[0] == '\0') ? uHash : HashConstI(&szString[1], (uHash ^ static_cast<FNV1A_t>(ToLower(szString[0]))) * PRIME);
	}

	/// compile-time hash for wide strings (case-sensitive)
	[[nodiscard]] consteval FNV1A_t HashConstW(const wchar_t* wszString, const FNV1A_t uHash = BASIS) noexcept
	{
		if (wszString[0] == L'\0')
			return uHash;

		// hash both bytes of wchar_t
		const auto c = static_cast<FNV1A_t>(wszString[0]);
		FNV1A_t h = (uHash ^ (c & 0xFF)) * PRIME;
		h = (h ^ ((c >> 8) & 0xFF)) * PRIME;
		return HashConstW(&wszString[1], h);
	}

	/* @section: runtime hashing */
	/// runtime hash for narrow strings (case-sensitive)
	[[nodiscard]] inline FNV1A_t Hash(const char* szString, FNV1A_t uHash = BASIS) noexcept
	{
		for (; *szString != '\0'; ++szString)
			uHash = (uHash ^ static_cast<FNV1A_t>(*szString)) * PRIME;
		return uHash;
	}

	/// runtime hash for narrow strings with explicit length
	[[nodiscard]] inline FNV1A_t Hash(const char* szString, const std::size_t nLength, FNV1A_t uHash = BASIS) noexcept
	{
		for (std::size_t i = 0; i < nLength; ++i)
			uHash = (uHash ^ static_cast<FNV1A_t>(szString[i])) * PRIME;
		return uHash;
	}

	/// runtime hash for narrow strings (case-insensitive)
	[[nodiscard]] inline FNV1A_t HashI(const char* szString, FNV1A_t uHash = BASIS) noexcept
	{
		for (; *szString != '\0'; ++szString)
			uHash = (uHash ^ static_cast<FNV1A_t>(ToLower(*szString))) * PRIME;
		return uHash;
	}

	/// runtime hash for wide strings (case-sensitive)
	[[nodiscard]] inline FNV1A_t HashW(const wchar_t* wszString, FNV1A_t uHash = BASIS) noexcept
	{
		for (; *wszString != L'\0'; ++wszString)
		{
			const auto c = static_cast<FNV1A_t>(*wszString);
			uHash = (uHash ^ (c & 0xFF)) * PRIME;
			uHash = (uHash ^ ((c >> 8) & 0xFF)) * PRIME;
		}
		return uHash;
	}

	/// runtime hash for wide strings (case-insensitive)
	[[nodiscard]] inline FNV1A_t HashWI(const wchar_t* wszString, FNV1A_t uHash = BASIS) noexcept
	{
		for (; *wszString != L'\0'; ++wszString)
		{
			const auto c = static_cast<FNV1A_t>(ToLowerW(*wszString));
			uHash = (uHash ^ (c & 0xFF)) * PRIME;
			uHash = (uHash ^ ((c >> 8) & 0xFF)) * PRIME;
		}
		return uHash;
	}
}

// compile-time hash macro — use in switch cases, template args, constexpr contexts
#define FNV1A_HASH(str)        FNV1A::HashConst(str)
#define FNV1A_HASH_I(str)      FNV1A::HashConstI(str)
#define FNV1A_HASH_W(str)      FNV1A::HashConstW(str)
