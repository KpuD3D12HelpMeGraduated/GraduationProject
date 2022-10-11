#pragma once
#include "Util.h"
class SwapChain
{
public:
	ComPtr<IDXGISwapChain> _swapChain;
	ComPtr<ID3D12Resource> _renderTargets[2];
	UINT _backBufferIndex = 0;

	//스왑체인 서술 및 생성
	void DescriptAndCreateSwapChain(WindowInfo windowInfo, shared_ptr<Device> devicePtr, shared_ptr<CmdQueue> cmdQueuePtr);
};