#include "Device.h"

void Device::CreateDevice()
{
	CreateDXGIFactory(IID_PPV_ARGS(&_dxgi)); //DXGI ����
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)); //����̽� ����
}