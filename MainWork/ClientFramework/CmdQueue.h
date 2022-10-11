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

	//Ŀ�ǵ� ����Ʈ, ť, �Ҵ��� ����
	void CreateCmdListAndCmdQueue(shared_ptr<Device> devicePtr);
	//cpu�� gpu ����ȭ
	void WaitSync();
	//���ҽ� Ŀ�ǵ�ť ������ cpu�� gpu ����ȭ
	void FlushResourceCommandQueue();
};