#include "Device.h"
#include "CmdQueue.h"
#include "SwapChain.h"

void SwapChain::DescriptAndCreateSwapChain(WindowInfo windowInfo, shared_ptr<Device> devicePtr, shared_ptr<CmdQueue> cmdQueuePtr)
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = static_cast<UINT>(windowInfo.ClientWidth); //버퍼의 해상도 너비
	sd.BufferDesc.Height = static_cast<UINT>(windowInfo.ClientHeight); //버퍼의 해상도 높이
	sd.BufferDesc.RefreshRate.Numerator = 60; //화면 갱신 비율
	sd.BufferDesc.RefreshRate.Denominator = 1; //화면 갱신 비율
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //버퍼의 디스플레이 형식
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1; //멀티 샘플링 OFF
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //후면 버퍼에 렌더링할 것 
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT; //전면+후면 버퍼
	sd.OutputWindow = windowInfo.hwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //전면 후면 버퍼 교체 시 이전 프레임 정보 버림
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	devicePtr->_dxgi->CreateSwapChain(cmdQueuePtr->_cmdQueue.Get(), &sd, &_swapChain);

	for (int i = 0; i < 2; i++)
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
}