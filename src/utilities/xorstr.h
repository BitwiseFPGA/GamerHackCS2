#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/*
 * COMPILE-TIME XOR STRING ENCRYPTION
 * - encrypts string literals at compile-time using XOR
 * - decrypts at runtime when accessed
 * - prevents static string analysis in the binary
 *
 * usage: _XS("my secret string")
 *        _XSW(L"my wide string")
 */

namespace XOR_DETAIL
{
	// compile-time seed from __LINE__ and __COUNTER__
	constexpr std::uint64_t SEED = (__LINE__ * 2654435761ull) ^ 0xDEADBEEFCAFEBABEull;

	// compile-time pseudo-random key generation
	constexpr std::uint8_t GenerateKey(const std::uint64_t uSeed, const std::size_t nIndex) noexcept
	{
		// FNV-1a inspired mixing
		std::uint64_t h = uSeed ^ (nIndex * 0x100000001B3ull);
		h ^= h >> 33;
		h *= 0xFF51AFD7ED558CCDull;
		h ^= h >> 33;
		h *= 0xC4CEB9FE1A85EC53ull;
		h ^= h >> 33;
		return static_cast<std::uint8_t>(h & 0xFF);
	}

	template <typename CharT, std::size_t N, std::uint64_t Key>
	class XorString
	{
	public:
		// encrypt at compile-time
		consteval XorString(const CharT(&str)[N]) noexcept
			: encrypted{}
		{
			for (std::size_t i = 0; i < N; ++i)
			{
				const auto key = static_cast<CharT>(GenerateKey(Key, i));
				encrypted[i] = str[i] ^ key;
			}
		}

		// decrypt at runtime — returns pointer to decrypted string
		const CharT* Decrypt() noexcept
		{
			if (!bDecrypted)
			{
				for (std::size_t i = 0; i < N; ++i)
				{
					const auto key = static_cast<CharT>(GenerateKey(Key, i));
					encrypted[i] ^= key;
				}
				bDecrypted = true;
			}
			return encrypted;
		}

		// auto-decrypt on access
		operator const CharT* () noexcept
		{
			return Decrypt();
		}

	private:
		CharT encrypted[N];
		bool bDecrypted = false;
	};
}

// unique key per usage site using __COUNTER__
#define _XOR_KEY(counter) (0x5A3C6E9D'1F7B4A82ull ^ ((counter) * 0x100000001B3ull) ^ (__LINE__ * 0x517CC1B727220A95ull))

// narrow string encryption
#define _XS(str) ([]() noexcept -> const char* {                                           \
    static XOR_DETAIL::XorString<char, sizeof(str), _XOR_KEY(__COUNTER__)> xored(str);     \
    return xored.Decrypt();                                                                 \
}())

// wide string encryption
#define _XSW(str) ([]() noexcept -> const wchar_t* {                                            \
    static XOR_DETAIL::XorString<wchar_t, sizeof(str)/sizeof(wchar_t), _XOR_KEY(__COUNTER__)> xored(str); \
    return xored.Decrypt();                                                                      \
}())
