#pragma once
#include "Util.h"
class RTV
{
public:
	ComPtr<ID3D12DescriptorHeap>	_rtvHeap;
	UINT							_rtvHeapSize = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE		_rtvHandle[2];

	//RTV»ý¼º
	void CreateRTV(shared_ptr<Device> devicePtr, shared_ptr<SwapChain> swapChainPtr);
};