#pragma once
#include "DxEngine.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

class Client
{
public:
	DxEngine dxEngine; //DX엔진
	WindowInfo windowInfo; //화면 관련 정보 객체

	void Init(HINSTANCE hInst, int nCmdShow)
	{
		//윈도우 객체 초기화
		WNDCLASS WndClass;
		WndClass.style = CS_HREDRAW | CS_VREDRAW;
		WndClass.lpfnWndProc = WndProc;
		WndClass.cbClsExtra = 0;
		WndClass.cbWndExtra = 0;
		WndClass.hInstance = hInst;
		WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		WndClass.lpszMenuName = NULL;
		WndClass.lpszClassName = _T("DXPractice");
		RegisterClass(&WndClass);
		windowInfo.hwnd = CreateWindow(_T("DXPractice"), _T("DXPractice"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowInfo.ClientWidth, windowInfo.ClientHeight, NULL, NULL, hInst, NULL);
		ShowWindow(windowInfo.hwnd, nCmdShow);
		UpdateWindow(windowInfo.hwnd);
		
		//엔진 초기화
		dxEngine.Init(windowInfo);

		//오브젝트 데이터 생성
		vector<Vertex> vec(4);
		vec[0].pos = XMFLOAT3(-0.5f, 0.5f, 0.0f);
		vec[0].color = XMFLOAT4(0.f, 0.f, 1.f, 1.f);
		vec[0].uv = XMFLOAT2(0.f, 0.f);
		vec[1].pos = XMFLOAT3(0.5f, 0.5f, 0.0f);
		vec[1].color = XMFLOAT4(0.f, 0.f, 1.f, 1.f);
		vec[1].uv = XMFLOAT2(1.f, 0.f);
		vec[2].pos = XMFLOAT3(0.5f, -0.5f, 0.0f);
		vec[2].color = XMFLOAT4(0.f, 0.f, 1.f, 1.f);
		vec[2].uv = XMFLOAT2(1.f, 1.f);
		vec[3].pos = XMFLOAT3(-0.5f, -0.5f, 0.0f);
		vec[3].color = XMFLOAT4(0.f, 0.f, 1.f, 1.f);
		vec[3].uv = XMFLOAT2(0.f, 1.f);
		vector<UINT> indexVec;
		{
			indexVec.push_back(0);
			indexVec.push_back(1);
			indexVec.push_back(2);
		}
		{
			indexVec.push_back(0);
			indexVec.push_back(2);
			indexVec.push_back(3);
		}
		dxEngine.vertexBufferPtr->CreateVertexBuffer(vec, dxEngine.devicePtr);
		dxEngine.indexBufferPtr->CreateIndexBuffer(indexVec, dxEngine.devicePtr);
		dxEngine.psoPtr->CreateInputLayoutAndPSOAndShader(dxEngine.devicePtr, dxEngine.rootSignaturePtr, dxEngine.dsvPtr);
		dxEngine.texturePtr->CreateTexture(L"..\\Resources\\Texture\\bricks.dds", dxEngine.devicePtr, dxEngine.cmdQueuePtr);
		dxEngine.texturePtr->CreateSRV(dxEngine.devicePtr);

		dxEngine.cmdQueuePtr->WaitSync();
	}

	int Update()
	{
		//매 프레임마다 업데이트
		MSG msg = { 0 };
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				//매 프레임마다 그리기
				dxEngine.Draw();
			}
		}

		return (int)msg.wParam;
	}
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}