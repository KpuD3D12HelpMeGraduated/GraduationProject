#pragma once
#include "Util.h"
class DSV
{
public:
	ComPtr<ID3D12Resource>				_dsvBuffer;
	ComPtr<ID3D12DescriptorHeap>		_dsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE			_dsvHandle = {};
	DXGI_FORMAT							_dsvFormat = {};
	
	//DSV»ý¼º
	void CreateDSV(DXGI_FORMAT dsvFormat, WindowInfo windowInfo, shared_ptr<Device> devicePtr);
};