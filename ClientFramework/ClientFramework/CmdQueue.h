#pragma once
#include "Util.h"
class CmdQueue
{
public:
	ComPtr<ID3D12CommandQueue>			_cmdQueue;
	ComPtr<ID3D12CommandAllocator>		_cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	_cmdList;
	ComPtr<ID3D12CommandAllocator>		_resCmdAlloc;
	ComPtr<ID3D12GraphicsCommandList>	_resCmdList;

	ComPtr<ID3D12Fence>					_fence;
	UINT								_fenceValue = 0;
	HANDLE								_fenceEvent = INVALID_HANDLE_VALUE;

	//커맨드 리스트, 큐, 할당자 생성
	void CreateCmdListAndCmdQueue(shared_ptr<Device> devicePtr);
	//cpu와 gpu 동기화
	void WaitSync();
	//리소스 커맨드큐 포함한 cpu와 gpu 동기화
	void FlushResourceCommandQueue();
};