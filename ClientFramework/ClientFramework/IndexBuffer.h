#pragma once
#include "Util.h"
class IndexBuffer
{
public:
	ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;
	ComPtr<ID3D12Resource> _npcIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW _npcIndexBufferView;
	UINT _indexCount = 0;

	//인덱스 버퍼 생성
	void CreateIndexBuffer(const vector<UINT>& buffer, shared_ptr<Device> devicePtr, int isPlayer); //isPlayer : 플레이어 로드할 지 npc로드할 지, 나중에 modelindex로도 구분 가능 할 수 있게 int로 해둠
};