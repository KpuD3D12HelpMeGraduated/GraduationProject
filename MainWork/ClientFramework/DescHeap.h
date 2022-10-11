#pragma once
#include "Util.h"
class DescHeap
{
public:
	ComPtr<ID3D12DescriptorHeap> _descHeap;
	UINT _handleSize = 0;
	UINT _groupSize = 0;
	UINT _groupCount = 0;
	UINT _currentGroupIndex = 0;

	void CreateDescTable(UINT count, shared_ptr<Device> devicePtr);

	void SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, UINT reg, shared_ptr<Device> devicePtr);

	void SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, UINT reg, shared_ptr<Device> devicePtr);

	void CommitTable(shared_ptr<CmdQueue> cmdQueuePtr);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT reg);
};