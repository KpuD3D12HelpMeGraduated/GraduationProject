#include "Device.h"
#include "VertexBuffer.h"

void VertexBuffer::CreateVertexBuffer(const vector<Vertex>& buffer, shared_ptr<Device> devicePtr, int isPlayer)
{
	_vertexCount = static_cast<UINT>(buffer.size());
	UINT bufferSize = _vertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	

	if (isPlayer == 1)
	{
		devicePtr->_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer));

		void* vertexDataBuffer = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		_vertexBuffer->Map(0, &readRange, &vertexDataBuffer);
		memcpy(vertexDataBuffer, &buffer[0], bufferSize);
		_vertexBuffer->Unmap(0, nullptr);

		_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
		_vertexBufferView.StrideInBytes = sizeof(Vertex);
		_vertexBufferView.SizeInBytes = bufferSize;
	}
	else
	{
		devicePtr->_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_npcVertexBuffer));

		void* vertexDataBuffer = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		_npcVertexBuffer->Map(0, &readRange, &vertexDataBuffer);
		memcpy(vertexDataBuffer, &buffer[0], bufferSize);
		_npcVertexBuffer->Unmap(0, nullptr);

		_npcVertexBufferView.BufferLocation = _npcVertexBuffer->GetGPUVirtualAddress();
		_npcVertexBufferView.StrideInBytes = sizeof(Vertex);
		_npcVertexBufferView.SizeInBytes = bufferSize;
	}
	
}