#include "Device.h"
#include "DSV.h"
#include "RootSignature.h"
#include "PSO.h"
void PSO::CreateInputLayoutAndPSOAndShader(shared_ptr<Device> devicePtr, shared_ptr<RootSignature> rootSignaturePtr, shared_ptr<DSV> dsvPtr)
{
	//셰이더 컴파일
	D3DCompileFromFile(L"..\\Shader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_Main", "vs_5_0", 0, 0, &_vsBlob, &_errBlob);
	_pipelineDesc.VS = { _vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize() };

	D3DCompileFromFile(L"..\\Shader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_Main", "ps_5_0", 0, 0, &_psBlob, &_errBlob);
	_pipelineDesc.PS = { _psBlob->GetBufferPointer(), _psBlob->GetBufferSize() };

	//인풋레이아웃 서술
	D3D12_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	//PSO 서술 및 생성
	_pipelineDesc.InputLayout = { desc, _countof(desc) };
	_pipelineDesc.pRootSignature = rootSignaturePtr->_signature.Get();

	_pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	_pipelineDesc.SampleMask = UINT_MAX;
	_pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_pipelineDesc.NumRenderTargets = 1;
	_pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	_pipelineDesc.SampleDesc.Count = 1;
	_pipelineDesc.DSVFormat = dsvPtr->_dsvFormat;

	devicePtr->_device->CreateGraphicsPipelineState(&_pipelineDesc, IID_PPV_ARGS(&_pipelineState));
}