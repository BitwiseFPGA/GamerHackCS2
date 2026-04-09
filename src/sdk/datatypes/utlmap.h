#pragma once
#include <optional>

// @source: master/public/tier1/utlmap.h
// Binary tree map — read-only view for reading game memory.

template<class K, class V>
class CUtlMap {
public:
    struct Node_t {
        int m_Left, m_Right, m_Parent, m_Tag;
        K m_Key;
        V m_Value;
    };

    class Iterator {
        Node_t* m_pCurrent;
        Node_t* m_pBase;
        int m_nCount;
        int m_nIndex;
    public:
        Iterator(Node_t* pBase, int nCount, int nIndex) : m_pBase(pBase), m_nCount(nCount), m_nIndex(nIndex) {
            m_pCurrent = (nIndex < nCount) ? &pBase[nIndex] : nullptr;
        }
        Node_t& operator*() { return *m_pCurrent; }
        Node_t* operator->() { return m_pCurrent; }
        Iterator& operator++() { ++m_nIndex; m_pCurrent = (m_nIndex < m_nCount) ? &m_pBase[m_nIndex] : nullptr; return *this; }
        bool operator!=(const Iterator& other) const { return m_nIndex != other.m_nIndex; }
        bool operator==(const Iterator& other) const { return m_nIndex == other.m_nIndex; }
    };

    std::optional<V> FindByKey(const K& key) const {
        for (int i = 0; i < m_nCount; ++i) {
            if (m_pElements[i].m_Key == key)
                return m_pElements[i].m_Value;
        }
        return std::nullopt;
    }

    Iterator begin() { return Iterator(m_pElements, m_nCount, 0); }
    Iterator end() { return Iterator(m_pElements, m_nCount, m_nCount); }

    int Count() const { return m_nCount; }

private:
    Node_t* m_pElements;
    int m_nCount;
};
