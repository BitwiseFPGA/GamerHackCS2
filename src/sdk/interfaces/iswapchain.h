#pragma once
#include <cstdint>
#include <dxgi.h>

class ISwapChainDx11
{
private:
	std::uint8_t _pad0[0x170]; // 0x0000
public:
	IDXGISwapChain* pDXGISwapChain; // 0x0170
};
