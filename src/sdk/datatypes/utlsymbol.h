#pragma once
#include <cstdint>

// @source: master/public/tier1/utlsymbol.h

class CUtlSymbol {
public:
    CUtlSymbol() : m_Id(0xFFFF) {}
    CUtlSymbol(uint16_t id) : m_Id(id) {}

    bool IsValid() const { return m_Id != 0xFFFF; }
    uint16_t GetID() const { return m_Id; }

    bool operator==(const CUtlSymbol& other) const { return m_Id == other.m_Id; }
    bool operator!=(const CUtlSymbol& other) const { return m_Id != other.m_Id; }

private:
    uint16_t m_Id;
};

class CUtlSymbolLarge {
public:
    CUtlSymbolLarge() : m_pString(nullptr) {}

    const char* String() const { return m_pString ? m_pString : ""; }
    bool IsValid() const { return m_pString != nullptr; }

    operator const char*() const { return String(); }

private:
    const char* m_pString;
};

class CUtlStringToken {
public:
    CUtlStringToken() : m_nHashCode(0) {}
    CUtlStringToken(uint32_t hashCode) : m_nHashCode(hashCode) {}

    uint32_t GetHashCode() const { return m_nHashCode; }
    void SetHashCode(uint32_t hashCode) { m_nHashCode = hashCode; }

    bool operator==(const CUtlStringToken& other) const { return m_nHashCode == other.m_nHashCode; }
    bool operator!=(const CUtlStringToken& other) const { return m_nHashCode != other.m_nHashCode; }
    bool operator<(const CUtlStringToken& other) const { return m_nHashCode < other.m_nHashCode; }

private:
    uint32_t m_nHashCode;
};

inline uint32_t MurmurHash2(const void* key, int len, uint32_t seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h = seed ^ len;
    const unsigned char* data = (const unsigned char*)key;
    while (len >= 4) {
        uint32_t k = *(uint32_t*)data;
        k *= m; k ^= k >> r; k *= m;
        h *= m; h ^= k;
        data += 4; len -= 4;
    }
    switch (len) {
    case 3: h ^= data[2] << 16; [[fallthrough]];
    case 2: h ^= data[1] << 8; [[fallthrough]];
    case 1: h ^= data[0]; h *= m;
    }
    h ^= h >> 13; h *= m; h ^= h >> 15;
    return h;
}

inline CUtlStringToken MakeStringToken(const char* str) {
    int len = 0;
    while (str[len]) ++len;
    return CUtlStringToken(MurmurHash2(str, len, 0x31415926));
}
