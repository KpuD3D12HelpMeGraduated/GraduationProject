#pragma once
#include "Util.h"
class Device
{
public:
	ComPtr<IDXGIFactory>		_dxgi;
	ComPtr<ID3D12Device>		_device;

	//디바이스 생성
	void CreateDevice();
};