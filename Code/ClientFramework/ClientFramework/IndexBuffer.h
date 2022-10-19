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

	//�ε��� ���� ����
	void CreateIndexBuffer(const vector<UINT>& buffer, shared_ptr<Device> devicePtr, int isPlayer); //isPlayer : �÷��̾� �ε��� �� npc�ε��� ��, ���߿� modelindex�ε� ���� ���� �� �� �ְ� int�� �ص�
};