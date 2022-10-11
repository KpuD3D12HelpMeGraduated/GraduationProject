#pragma once
#include "Util.h"
class ConstantBuffer
{
public:
	ComPtr<ID3D12Resource>	_cbvBuffer;
	BYTE* _mappedBuffer = nullptr;
	UINT _elementSize = 0;
	UINT _elementCount = 0;

	UINT _currentIndex = 0;

	ComPtr<ID3D12DescriptorHeap> _cbvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE	 _cpuHandleBegin = {};
	UINT _handleIncrementSize = 0;

	//��� ���� ����
	void CreateConstantBuffer(UINT size, UINT count, shared_ptr<Device> devicePtr);
	//CBV ����
	void CreateView(shared_ptr<Device> devicePtr);
	//��� ���ۿ� ������ ����
	D3D12_CPU_DESCRIPTOR_HANDLE PushData(int rootParamIndex, void* buffer, UINT size);

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT index);
};