#pragma once
#include <cstdint>
#include <cstring>
#include <new>

#include "utlstring.h"

// @source: master/public/tier1/utlvector.h
// Read-only view of Valve's CUtlVector for reading game memory.
// Does NOT perform allocations — safe to use on game-side memory.

template <typename T>
class CUtlVector
{
public:
	CUtlVector() = default;

	[[nodiscard]] T& operator[](int i) { return m_pMemory[i]; }
	[[nodiscard]] const T& operator[](int i) const { return m_pMemory[i]; }

	[[nodiscard]] T& Element(int i) { return m_pMemory[i]; }
	[[nodiscard]] const T& Element(int i) const { return m_pMemory[i]; }

	[[nodiscard]] T* Base() { return m_pMemory; }
	[[nodiscard]] const T* Base() const { return m_pMemory; }

	[[nodiscard]] T& Head() { return m_pMemory[0]; }
	[[nodiscard]] const T& Head() const { return m_pMemory[0]; }
	[[nodiscard]] T& Tail() { return m_pMemory[m_nSize - 1]; }
	[[nodiscard]] const T& Tail() const { return m_pMemory[m_nSize - 1]; }

	[[nodiscard]] int Count() const { return m_nSize; }
	[[nodiscard]] int Size() const { return m_nSize; }
	[[nodiscard]] bool IsEmpty() const { return m_nSize == 0; }
	[[nodiscard]] int Capacity() const { return m_nAllocationCount; }

	[[nodiscard]] bool IsValidIndex(int i) const { return (i >= 0) && (i < m_nSize); }
	static int InvalidIndex() { return -1; }

	[[nodiscard]] auto begin() noexcept { return m_pMemory; }
	[[nodiscard]] auto end() noexcept { return m_pMemory + m_nSize; }
	[[nodiscard]] auto begin() const noexcept { return m_pMemory; }
	[[nodiscard]] auto end() const noexcept { return m_pMemory + m_nSize; }

	// --- mutation (for local allocations only, NOT safe on game-side memory) ---
	void GrowVector(int nCount = 1)
	{
		if (m_nSize + nCount > m_nAllocationCount)
		{
			int nNewCount = m_nAllocationCount ? (m_nAllocationCount * 2) : 4;
			while (nNewCount < m_nSize + nCount)
				nNewCount *= 2;

			T* pNew = static_cast<T*>(::operator new(nNewCount * sizeof(T)));
			if (m_pMemory)
			{
				std::memcpy(pNew, m_pMemory, m_nSize * sizeof(T));
				::operator delete(m_pMemory);
			}
			m_pMemory = pNew;
			m_nAllocationCount = nNewCount;
		}
	}

	int AddToTail()
	{
		GrowVector();
		new (&m_pMemory[m_nSize]) T();
		return m_nSize++;
	}

	int AddToTail(const T& src)
	{
		GrowVector();
		new (&m_pMemory[m_nSize]) T(src);
		return m_nSize++;
	}

	int AddToHead()
	{
		return InsertBefore(0);
	}

	int InsertBefore(int nIndex)
	{
		GrowVector();
		if (nIndex < m_nSize)
			std::memmove(&m_pMemory[nIndex + 1], &m_pMemory[nIndex], (m_nSize - nIndex) * sizeof(T));
		new (&m_pMemory[nIndex]) T();
		++m_nSize;
		return nIndex;
	}

	int InsertBefore(int nIndex, const T& src)
	{
		GrowVector();
		if (nIndex < m_nSize)
			std::memmove(&m_pMemory[nIndex + 1], &m_pMemory[nIndex], (m_nSize - nIndex) * sizeof(T));
		new (&m_pMemory[nIndex]) T(src);
		++m_nSize;
		return nIndex;
	}

	int Find(const T& src) const
	{
		for (int i = 0; i < m_nSize; ++i)
			if (m_pMemory[i] == src) return i;
		return -1;
	}

	bool HasElement(const T& src) const { return Find(src) >= 0; }

	void FastRemove(int nIndex)
	{
		m_pMemory[nIndex].~T();
		if (nIndex < m_nSize - 1)
			std::memcpy(&m_pMemory[nIndex], &m_pMemory[m_nSize - 1], sizeof(T));
		--m_nSize;
	}

	void Remove(int nIndex)
	{
		m_pMemory[nIndex].~T();
		if (nIndex < m_nSize - 1)
			std::memmove(&m_pMemory[nIndex], &m_pMemory[nIndex + 1], (m_nSize - nIndex - 1) * sizeof(T));
		--m_nSize;
	}

	void RemoveAll()
	{
		for (int i = 0; i < m_nSize; ++i)
			m_pMemory[i].~T();
		m_nSize = 0;
	}

	void Purge()
	{
		RemoveAll();
		::operator delete(m_pMemory);
		m_pMemory = nullptr;
		m_nAllocationCount = 0;
	}

	void PurgeAndDeleteElements()
	{
		for (int i = 0; i < m_nSize; ++i)
			delete m_pMemory[i];
		Purge();
	}

protected:
	T*  m_pMemory = nullptr;
	int m_nAllocationCount = 0;
	int m_nGrowSize = 0;
	int m_nSize = 0;
};

/// fixed-capacity CUtlVector (stack allocated, no heap)
template <typename T, int MAX_SIZE>
class CUtlVectorFixed
{
public:
	CUtlVectorFixed() : m_nSize(0) {}

	[[nodiscard]] T& operator[](int i) { return reinterpret_cast<T*>(m_Data)[i]; }
	[[nodiscard]] const T& operator[](int i) const { return reinterpret_cast<const T*>(m_Data)[i]; }
	[[nodiscard]] T* Base() { return reinterpret_cast<T*>(m_Data); }
	[[nodiscard]] int Count() const { return m_nSize; }
	[[nodiscard]] bool IsEmpty() const { return m_nSize == 0; }

	int AddToTail(const T& src)
	{
		if (m_nSize >= MAX_SIZE) return -1;
		new (&reinterpret_cast<T*>(m_Data)[m_nSize]) T(src);
		return m_nSize++;
	}

	void RemoveAll()
	{
		for (int i = 0; i < m_nSize; ++i)
			reinterpret_cast<T*>(m_Data)[i].~T();
		m_nSize = 0;
	}

private:
	alignas(T) std::uint8_t m_Data[MAX_SIZE * sizeof(T)];
	int m_nSize;
};

/// fixed initial capacity that can grow onto the heap
template <typename T, int INITIAL_SIZE>
class CUtlVectorFixedGrowable : public CUtlVector<T>
{
public:
	CUtlVectorFixedGrowable()
	{
		this->m_pMemory = reinterpret_cast<T*>(m_FixedData);
		this->m_nAllocationCount = INITIAL_SIZE;
	}

private:
	alignas(T) std::uint8_t m_FixedData[INITIAL_SIZE * sizeof(T)];
};

/// network variant of CUtlVector used in schema-generated classes
template <typename T>
class C_NetworkUtlVectorBase
{
public:
	[[nodiscard]] T& operator[](int i) { return pElements[i]; }
	[[nodiscard]] const T& operator[](int i) const { return pElements[i]; }

	[[nodiscard]] int Count() const { return static_cast<int>(nSize); }
	[[nodiscard]] bool IsEmpty() const { return nSize == 0; }

	[[nodiscard]] auto begin() noexcept { return pElements; }
	[[nodiscard]] auto end() noexcept { return pElements + nSize; }
	[[nodiscard]] auto begin() const noexcept { return pElements; }
	[[nodiscard]] auto end() const noexcept { return pElements + nSize; }

	std::uint32_t nSize;
	T*            pElements;
};

/// alias used in some schema-generated headers
template <typename T>
using CNetworkUtlVectorBase = C_NetworkUtlVectorBase<T>;
