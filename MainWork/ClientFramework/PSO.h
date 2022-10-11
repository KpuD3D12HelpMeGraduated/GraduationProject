#pragma once
#include "Util.h"
class PSO
{
public:
	ComPtr<ID3DBlob> _vsBlob;
	ComPtr<ID3DBlob> _psBlob;
	ComPtr<ID3DBlob> _errBlob;

	ComPtr<ID3D12PipelineState> _pipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _pipelineDesc = {};

	//인풋레이아웃과 PSO생성 및 셰이더 컴파일
	void CreateInputLayoutAndPSOAndShader(shared_ptr<Device> devicePtr, shared_ptr<RootSignature> rootSignaturePtr, shared_ptr<DSV> dsvPtr);
};