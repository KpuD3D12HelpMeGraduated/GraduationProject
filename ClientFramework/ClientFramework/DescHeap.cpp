#include "Device.h"
#include "CmdQueue.h"
#include "DescHeap.h"

void DescHeap::CreateDescTable(UINT count, shared_ptr<Device> devicePtr)
{
	_groupCount = count;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = count * REGISTER_COUNT;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	devicePtr->_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_descHeap));

	_handleSize = devicePtr->_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_groupSize = _handleSize * REGISTER_COUNT;
}

void DescHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, UINT reg, shared_ptr<Device> devicePtr)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	UINT destRange = 1;
	UINT srcRange = 1;
	devicePtr->_device->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DescHeap::SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, UINT reg, shared_ptr<Device> devicePtr)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	UINT destRange = 1;
	UINT srcRange = 1;
	devicePtr->_device->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DescHeap::CommitTable(shared_ptr<CmdQueue> cmdQueuePtr)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = _descHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += _currentGroupIndex * _groupSize;
	cmdQueuePtr->_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	_currentGroupIndex++;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescHeap::GetCPUHandle(UINT reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = _descHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += _currentGroupIndex * _groupSize;
	handle.ptr += reg * _handleSize;
	return handle;
}