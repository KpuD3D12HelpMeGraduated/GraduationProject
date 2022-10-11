#pragma once
#include "Util.h"
class Texture
{
public:
	ScratchImage _image;
	ComPtr<ID3D12Resource> _tex2D;

	ComPtr<ID3D12DescriptorHeap> _srvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE	 _srvHandle;

	//텍스처 로드 및 업로드
	void CreateTexture(const wstring& path, shared_ptr<Device> devicePtr, shared_ptr<CmdQueue> cmdQueuePtr);

	//SRV 생성
	void CreateSRV(shared_ptr<Device> devicePtr);
};