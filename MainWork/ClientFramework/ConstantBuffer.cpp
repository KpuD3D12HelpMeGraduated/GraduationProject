#include "Device.h"
#include "ConstantBuffer.h"
void ConstantBuffer::CreateConstantBuffer(UINT size, UINT count, shared_ptr<Device> devicePtr)
{
	_elementSize = (size + 255) & ~255;
	_elementCount = count;

	UINT bufferSize = _elementSize * _elementCount;
	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	devicePtr->_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_cbvBuffer));

	_cbvBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_mappedBuffer));
}

void ConstantBuffer::CreateView(shared_ptr<Device> devicePtr)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvDesc = {};
	cbvDesc.NumDescriptors = _elementCount;
	cbvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	devicePtr->_device->CreateDescriptorHeap(&cbvDesc, IID_PPV_ARGS(&_cbvHeap));

	_cpuHandleBegin = _cbvHeap->GetCPUDescriptorHandleForHeapStart();
	_handleIncrementSize = devicePtr->_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (UINT i = 0; i < _elementCount; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cpuHandleBegin, i * _handleIncrementSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = _cbvBuffer->GetGPUVirtualAddress() + static_cast<UINT>(_elementSize) * i;
		cbvDesc.SizeInBytes = _elementSize;   // CB size is required to be 256-byte aligned.

		devicePtr->_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE ConstantBuffer::PushData(int rootParamIndex, void* buffer, UINT size)
{
	assert(_currentIndex < _elementSize);

	memcpy(&_mappedBuffer[_currentIndex * _elementSize], buffer, size);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(_cpuHandleBegin, _currentIndex * _handleIncrementSize);
	_currentIndex++;

	return cpuHandle;
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGpuVirtualAddress(UINT index)
{
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = _cbvBuffer->GetGPUVirtualAddress();
	objCBAddress += index * _elementSize;
	return objCBAddress;
}