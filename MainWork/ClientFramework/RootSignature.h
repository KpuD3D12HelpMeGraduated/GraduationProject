#pragma once
#include "Util.h"
class RootSignature
{
public:
	ComPtr<ID3D12RootSignature>	_signature;
	D3D12_STATIC_SAMPLER_DESC _samplerDesc;

	//风飘 矫弊聪贸 积己
	void CreateRootSignature(shared_ptr<Device> devicePtr);
};