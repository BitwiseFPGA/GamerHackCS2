#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// GCSDK types — Game Coordinator shared object identifiers
// ---------------------------------------------------------------
namespace GCSDK
{
	// ---------------------------------------------------------------
	// SOID_t — shared object identifier
	// ---------------------------------------------------------------
	struct SOID_t
	{
		SOID_t()
			: m_id(0), m_type(0), m_padding(0) {}

		SOID_t(std::uint32_t type, std::uint64_t id)
			: m_id(id), m_type(type), m_padding(0) {}

		void Init(std::uint32_t type, std::uint64_t id)
		{
			m_type = type;
			m_id   = id;
		}

		[[nodiscard]] std::uint64_t ID() const { return m_id; }
		[[nodiscard]] std::uint32_t Type() const { return m_type; }

		[[nodiscard]] bool IsValid() const { return m_type != 0; }

		bool operator==(const SOID_t& rhs) const { return m_type == rhs.m_type && m_id == rhs.m_id; }
		bool operator!=(const SOID_t& rhs) const { return !(*this == rhs); }

		bool operator<(const SOID_t& rhs) const
		{
			if (m_type == rhs.m_type)
				return m_id < rhs.m_id;
			return m_type < rhs.m_type;
		}

		std::uint64_t m_id;
		std::uint32_t m_type;
		std::uint32_t m_padding;
	};

	// ---------------------------------------------------------------
	// ESOCacheEvent — shared object cache event types
	// ---------------------------------------------------------------
	enum ESOCacheEvent
	{
		eSOCacheEvent_None            = 0,
		eSOCacheEvent_Subscribed      = 1,
		eSOCacheEvent_Unsubscribed    = 2,
		eSOCacheEvent_Resubscribed    = 3,
		eSOCacheEvent_Incremental     = 4,
		eSOCacheEvent_ListenerAdded   = 5,
		eSOCacheEvent_ListenerRemoved = 6,
	};
}
