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

	//���ؽ� ���� ����
	void CreateVertexBuffer(const vector<Vertex>& buffer, shared_ptr<Device> devicePtr, int isPlayer); //isPlayer : �÷��̾� �ε��� �� npc�ε��� ��, ���߿� modelindex�ε� ���� ���� �� �� �ְ� int�� �ص�
};