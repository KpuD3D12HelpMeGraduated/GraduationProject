#include "DxEngine.h"
#include "Device.h"

void DxEngine::Init(WindowInfo windowInfo)
{
	//화면 크기 설정
	_viewport = { 0, 0, static_cast<FLOAT>(windowInfo.ClientWidth), static_cast<FLOAT>(windowInfo.ClientHeight), 0.0f, 1.0f };
	_scissorRect = CD3DX12_RECT(0, 0, windowInfo.ClientWidth, windowInfo.ClientHeight);

	//DX엔진 초기화
	devicePtr->CreateDevice();
	cmdQueuePtr->CreateCmdListAndCmdQueue(devicePtr);
	swapChainPtr->DescriptAndCreateSwapChain(windowInfo, devicePtr, cmdQueuePtr);
	rtvPtr->CreateRTV(devicePtr, swapChainPtr);
	cameraPtr->TransformProjection(windowInfo); //투영 변환
	rootSignaturePtr->CreateRootSignature(devicePtr);
	constantBufferPtr->CreateConstantBuffer(sizeof(Constants), 256, devicePtr);
	constantBufferPtr->CreateView(devicePtr);
	descHeapPtr->CreateDescTable(256, devicePtr);
	dsvPtr->CreateDSV(DXGI_FORMAT_D32_FLOAT, windowInfo, devicePtr);
	RECT rect = { 0, 0, windowInfo.ClientWidth, windowInfo.ClientHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(windowInfo.hwnd, 0, 100, 100, windowInfo.ClientWidth, windowInfo.ClientHeight, 0);
	dsvPtr->CreateDSV(DXGI_FORMAT_D32_FLOAT, windowInfo, devicePtr);
}

void DxEngine::Draw()
{
	//WVP 변환
	XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up); //뷰 변환 행렬
	XMStoreFloat4x4(&cameraPtr->mView, view);
	XMMATRIX world = XMLoadFloat4x4(&cameraPtr->mWorld); //월드 변환 행렬
	XMMATRIX proj = XMLoadFloat4x4(&cameraPtr->mProj); //투영 변환 행렬
	XMMATRIX worldViewProj = world * view * proj;
	XMStoreFloat4x4(&vertexBufferPtr->_transform.worldViewProj, XMMatrixTranspose(worldViewProj));

	//렌더 시작
	cmdQueuePtr->_cmdAlloc->Reset();
	cmdQueuePtr->_cmdList->Reset(cmdQueuePtr->_cmdAlloc.Get(), nullptr);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainPtr->_renderTargets[swapChainPtr->_backBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	cmdQueuePtr->_cmdList->SetGraphicsRootSignature(rootSignaturePtr->_signature.Get());
	constantBufferPtr->_currentIndex = 0;
	descHeapPtr->_currentGroupIndex = 0;

	ID3D12DescriptorHeap* descHeap = descHeapPtr->_descHeap.Get();
	cmdQueuePtr->_cmdList->SetDescriptorHeaps(1, &descHeap);

	cmdQueuePtr->_cmdList->ResourceBarrier(1, &barrier);

	cmdQueuePtr->_cmdList->RSSetViewports(1, &_viewport);
	cmdQueuePtr->_cmdList->RSSetScissorRects(1, &_scissorRect);

	D3D12_CPU_DESCRIPTOR_HANDLE backBufferView = rtvPtr->_rtvHandle[swapChainPtr->_backBufferIndex];
	cmdQueuePtr->_cmdList->ClearRenderTargetView(backBufferView, Colors::LightSteelBlue, 0, nullptr);
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = dsvPtr->_dsvHandle;
	cmdQueuePtr->_cmdList->OMSetRenderTargets(1, &backBufferView, FALSE, &depthStencilView);

	cmdQueuePtr->_cmdList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	cmdQueuePtr->_cmdList->SetPipelineState(psoPtr->_pipelineState.Get());

	//렌더
	{
		cmdQueuePtr->_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdQueuePtr->_cmdList->IASetVertexBuffers(0, 1, &vertexBufferPtr->_vertexBufferView);
		cmdQueuePtr->_cmdList->IASetIndexBuffer(&indexBufferPtr->_indexBufferView);
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = constantBufferPtr->PushData(0, &vertexBufferPtr->_transform, sizeof(vertexBufferPtr->_transform));
			descHeapPtr->SetCBV(handle, 0, devicePtr);
			descHeapPtr->SetSRV(texturePtr->_srvHandle, 5, devicePtr);
		}

		descHeapPtr->CommitTable(cmdQueuePtr);
		cmdQueuePtr->_cmdList->DrawIndexedInstanced(indexBufferPtr->_indexCount, 1, 0, 0, 0);
	}

	//렌더 종료
	D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(swapChainPtr->_renderTargets[swapChainPtr->_backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT); // 화면 출력

	cmdQueuePtr->_cmdList->ResourceBarrier(1, &barrier2);
	cmdQueuePtr->_cmdList->Close();

	ID3D12CommandList* cmdListArr[] = { cmdQueuePtr->_cmdList.Get() };
	cmdQueuePtr->_cmdQueue->ExecuteCommandLists(_countof(cmdListArr), cmdListArr);

	swapChainPtr->_swapChain->Present(0, 0);

	cmdQueuePtr->WaitSync();

	swapChainPtr->_backBufferIndex = (swapChainPtr->_backBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}