#pragma once
#include "Util.h"

class VertexBuffer
{
public:
	ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};
	ComPtr<ID3D12Resource> _npcVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _npcVertexBufferView = {};
	UINT _vertexCount = 0;
	Constants _transform = {};

	//버텍스 버퍼 생성
	void CreateVertexBuffer(const vector<Vertex>& buffer, shared_ptr<Device> devicePtr, int isPlayer); //isPlayer : 플레이어 로드할 지 npc로드할 지, 나중에 modelindex로도 구분 가능 할 수 있게 int로 해둠
};