#pragma once
#include "Util.h"

class VertexBuffer
{
public:
	ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};
	UINT _vertexCount = 0;
	Constants _transform = {};

	//버텍스 버퍼 생성
	void CreateVertexBuffer(const vector<Vertex>& buffer, shared_ptr<Device> devicePtr);
};