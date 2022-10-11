#include "Device.h"
#include "CmdQueue.h"
#include "SwapChain.h"
#include "RTV.h"

void RTV::CreateRTV(shared_ptr<Device> devicePtr, shared_ptr<SwapChain> swapChainPtr)
{
	_rtvHeapSize = devicePtr->_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0;

	devicePtr->_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapBegin = _rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 2; i++)
	{
		_rtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeapBegin, i * _rtvHeapSize);
		devicePtr->_device->CreateRenderTargetView(swapChainPtr->_renderTargets[i].Get(), nullptr, _rtvHandle[i]);
	}
}