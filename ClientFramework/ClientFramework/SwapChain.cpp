#include "Device.h"
#include "CmdQueue.h"
#include "SwapChain.h"

void SwapChain::DescriptAndCreateSwapChain(WindowInfo windowInfo, shared_ptr<Device> devicePtr, shared_ptr<CmdQueue> cmdQueuePtr)
{
	_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = static_cast<UINT>(windowInfo.ClientWidth); //������ �ػ� �ʺ�
	sd.BufferDesc.Height = static_cast<UINT>(windowInfo.ClientHeight); //������ �ػ� ����
	sd.BufferDesc.RefreshRate.Numerator = 60; //ȭ�� ���� ����
	sd.BufferDesc.RefreshRate.Denominator = 1; //ȭ�� ���� ����
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //������ ���÷��� ����
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1; //��Ƽ ���ø� OFF
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //�ĸ� ���ۿ� �������� �� 
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT; //����+�ĸ� ����
	sd.OutputWindow = windowInfo.hwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //���� �ĸ� ���� ��ü �� ���� ������ ���� ����
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	devicePtr->_dxgi->CreateSwapChain(cmdQueuePtr->_cmdQueue.Get(), &sd, &_swapChain);

	for (int i = 0; i < 2; i++)
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
}