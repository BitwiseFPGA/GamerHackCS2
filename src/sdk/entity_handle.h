#pragma once
#include <cstdint>

// @source: https://developer.valvesoftware.com/wiki/Entity_limit#Source_2_limits
inline constexpr std::uint32_t INVALID_EHANDLE_INDEX  = 0xFFFFFFFF;
inline constexpr std::uint32_t ENT_ENTRY_MASK         = 0x7FFF;
inline constexpr int           NUM_SERIAL_NUM_SHIFT_BITS = 15;
inline constexpr int           ENT_MAX_NETWORKED_ENTRY   = 16384;

class CBaseHandle
{
public:
	CBaseHandle() noexcept :
		m_nIndex(INVALID_EHANDLE_INDEX) { }

	CBaseHandle(int nEntry, int nSerial) noexcept
	{
		m_nIndex = static_cast<std::uint32_t>(nEntry) |
		           (static_cast<std::uint32_t>(nSerial) << NUM_SERIAL_NUM_SHIFT_BITS);
	}

	bool operator==(const CBaseHandle& o) const noexcept { return m_nIndex == o.m_nIndex; }
	bool operator!=(const CBaseHandle& o) const noexcept { return m_nIndex != o.m_nIndex; }
	bool operator<(const CBaseHandle& o) const noexcept  { return m_nIndex < o.m_nIndex; }

	[[nodiscard]] bool IsValid() const noexcept
	{
		return m_nIndex != INVALID_EHANDLE_INDEX;
	}

	[[nodiscard]] int GetEntryIndex() const noexcept
	{
		return static_cast<int>(m_nIndex & ENT_ENTRY_MASK);
	}

	[[nodiscard]] int GetSerialNumber() const noexcept
	{
		return static_cast<int>(m_nIndex >> NUM_SERIAL_NUM_SHIFT_BITS);
	}

	[[nodiscard]] std::uint32_t GetRawIndex() const noexcept { return m_nIndex; }

	explicit operator bool() const noexcept { return IsValid(); }

private:
	std::uint32_t m_nIndex;
};

/// typed entity handle — T is the entity class the handle refers to
template <typename T = void>
class CHandle : public CBaseHandle
{
public:
	CHandle() noexcept = default;

	CHandle(int nEntry, int nSerial) noexcept :
		CBaseHandle(nEntry, nSerial) { }

	/// resolve handle to entity pointer (implemented after entity system is available)
	[[nodiscard]] T* Get() const;

	T* operator->() const { return Get(); }
	operator T*() const { return Get(); }
};
