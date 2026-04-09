#pragma once
#include <cstring>
#include <cassert>

// @source: master/public/tier1/utlmemory.h

template<class T, class I = int>
class CUtlMemory {
public:
    CUtlMemory(int nGrowSize = 0, int nInitSize = 0) : m_pMemory(nullptr), m_nAllocationCount(0), m_nGrowSize(nGrowSize) {
        if (nInitSize) Grow(nInitSize);
    }
    ~CUtlMemory() { Purge(); }

    class Iterator_t {
    public:
        Iterator_t(I i) : m_Index(i) {}
        I m_Index;
        bool operator==(const Iterator_t& it) const { return m_Index == it.m_Index; }
        bool operator!=(const Iterator_t& it) const { return m_Index != it.m_Index; }
    };

    T& operator[](I i) { return m_pMemory[i]; }
    const T& operator[](I i) const { return m_pMemory[i]; }

    T* Base() { return m_pMemory; }
    const T* Base() const { return m_pMemory; }

    int NumAllocated() const { return m_nAllocationCount; }
    bool IsExternallyAllocated() const { return m_nGrowSize < 0; }

    void Grow(int nCount = 1) {
        if (IsExternallyAllocated()) return;
        int nNewAllocationCount = m_nAllocationCount + nCount;
        if (m_pMemory) {
            T* pNewMemory = new T[nNewAllocationCount];
            std::memcpy(pNewMemory, m_pMemory, m_nAllocationCount * sizeof(T));
            delete[] m_pMemory;
            m_pMemory = pNewMemory;
        } else {
            m_pMemory = new T[nNewAllocationCount];
        }
        m_nAllocationCount = nNewAllocationCount;
    }

    void Purge() {
        if (!IsExternallyAllocated()) {
            delete[] m_pMemory;
            m_pMemory = nullptr;
            m_nAllocationCount = 0;
        }
    }

    void Init(T* pMemory, int nCount) {
        Purge();
        m_pMemory = pMemory;
        m_nAllocationCount = nCount;
        m_nGrowSize = -1; // externally allocated
    }

protected:
    T* m_pMemory;
    int m_nAllocationCount;
    int m_nGrowSize;
};

template<class T, size_t SIZE, class I = int>
class CUtlMemoryFixedGrowable : public CUtlMemory<T, I> {
    T m_FixedMemory[SIZE];
public:
    CUtlMemoryFixedGrowable(int nGrowSize = 0) : CUtlMemory<T, I>(nGrowSize, SIZE) {
        this->Init(m_FixedMemory, SIZE);
    }
};

template<class T>
inline T* Construct(T* pMemory) {
    return ::new(pMemory) T;
}

template<class T>
inline void Destruct(T* pMemory) {
    pMemory->~T();
}
