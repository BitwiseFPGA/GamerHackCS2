#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// resource binding used by CStrongHandle
// ---------------------------------------------------------------
struct ResourceBindingBase_t
{
	void* data;
};

// ---------------------------------------------------------------
// CStrongHandle<T> — strong reference to a resource
// ---------------------------------------------------------------
template <class T>
class CStrongHandle
{
public:
	explicit operator T*() const
	{
		return IsValid() ? reinterpret_cast<T*>(m_pBinding->data) : nullptr;
	}

	T* operator->() const
	{
		return IsValid() ? reinterpret_cast<T*>(m_pBinding->data) : nullptr;
	}

	[[nodiscard]] bool IsValid() const { return (m_pBinding && m_pBinding->data != nullptr); }

private:
	const ResourceBindingBase_t* m_pBinding;
};
