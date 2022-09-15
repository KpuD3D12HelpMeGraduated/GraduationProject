#include "dx.h"

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}

DX::DX(HINSTANCE hInstance) : mHAppInst(hInstance)
{
	assert(mApp == nullptr);
	mApp = this;

	mKeyStates.resize(255, 0);
}

DX::~DX()
{
	if (mD3dDevice != nullptr)
		FlushCommandQueue();
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DX::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

DX* DX::mApp = nullptr;
DX* DX::GetApp()
{
	return mApp;
}

bool DX::InitWinApi()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mHAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	RegisterClass(&wc);

	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mHMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mHAppInst, 0);

	ShowWindow(mHMainWnd, SW_SHOW);
	UpdateWindow(mHMainWnd);

	AllocConsole();
	freopen("CONOUT$", "wt", stdout);

	return true;
}

void DX::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		CreateSwapChain();
		OnResize();
	}
}

int DX::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				ReceiveServer();
				Update(mTimer);
				Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

void DX::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	mD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	mD3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()));
}

void DX::OnResize()
{
	assert(mD3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	FlushCommandQueue();

	mCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr);

	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	mSwapChain->ResizeBuffers(SwapChainBufferCount, mClientWidth, mClientHeight, mBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
		mD3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	mD3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf()));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	mD3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	mCmdList->Close();
	ID3D12CommandList* cmdsLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

LRESULT DX::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	case WM_SIZE:
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (mD3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
				}
				else
				{
					OnResize();
				}
			}
		}
		return 0;
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_CLOSE:
		FreeConsole();
		break;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool DX::InitDirect3D()
{
	CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory));

	HRESULT hardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3dDevice));

	mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));

	mRtvDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	mD3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	OnResize();

	mCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr);

	mCbvSrvDescSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < PLAYERMAX; i++)
	{
		LoadTextures(mPlayers, mPlayers[i], i);
	}
	for (int i = 0; i < OBJMAX; i++)
	{
		LoadTextures(mObjs, mObjs[i], i);
	}
	BuildDescriptorHeaps(PLAYERMAX + OBJMAX);
	BuildConstantBuffers();
	BuildConstantBuffers2();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	for (int i = 0; i < PLAYERMAX; i++)
	{
		BuildBoxGeometry(mPlayers, mPlayers[i], i);
	}
	for (int i = 0; i < OBJMAX; i++)
	{
		if (i == 0)
		{
			BuildLandGeometry(0);
		}
		else
		{
			BuildBoxGeometry(mObjs, mObjs[i], i);
		}
	}

	BuildPSO();

	mCmdList->Close();
	ID3D12CommandList* cmdsLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void DX::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	mD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue));

	mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf()));

	mD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(mCmdList.GetAddressOf()));

	mCmdList->Close();
}

void DX::CreateSwapChain()
{
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mHMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	mDxgiFactory->CreateSwapChain(mCmdQueue.Get(), &sd, mSwapChain.GetAddressOf());
}

void DX::FlushCommandQueue()
{
	mCurrentFence++;

	mCmdQueue->Signal(mFence.Get(), mCurrentFence);

	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		mFence->SetEventOnCompletion(mCurrentFence, eventHandle);

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* DX::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void DX::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mHMainWnd, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DX::Update(const GameTimer& gt)
{
	Input();

	if (mKeyStates['W'] == 1)
	{
		mPlayers[mMyClientId].mPos.x += 5.f * gt.DeltaTime() * cosf(mTheta);
		mPlayers[mMyClientId].mPos.z += 5.f * gt.DeltaTime() * sinf(mTheta);
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI / 2.0f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['S'] == 1)
	{
		mPlayers[mMyClientId].mPos.x -= 5.f * gt.DeltaTime() * cosf(mTheta);
		mPlayers[mMyClientId].mPos.z -= 5.f * gt.DeltaTime() * sinf(mTheta);
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI * 1.5f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['A'] == 1)
	{
		mPlayers[mMyClientId].mPos.x -= 5.f * gt.DeltaTime() * cosf(mTheta - XM_PI / 2.0f);
		mPlayers[mMyClientId].mPos.z -= 5.f * gt.DeltaTime() * sinf(mTheta - XM_PI / 2.0f);
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['D'] == 1)
	{
		mPlayers[mMyClientId].mPos.x += 5.f * gt.DeltaTime() * cosf(mTheta - XM_PI / 2.0f);
		mPlayers[mMyClientId].mPos.z += 5.f * gt.DeltaTime() * sinf(mTheta - XM_PI / 2.0f);
		mPlayers[mMyClientId].mRotation.y = -mTheta;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['W'] == 1 && mKeyStates['A'] == 1)
	{
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI * 0.75f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['W'] == 1 && mKeyStates['D'] == 1)
	{
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI * 0.25f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['S'] == 1 && mKeyStates['A'] == 1)
	{
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI * 1.25f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
	else if (mKeyStates['S'] == 1 && mKeyStates['D'] == 1)
	{
		mPlayers[mMyClientId].mRotation.y = -mTheta - XM_PI * 1.75f;

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.degree = mPlayers[mMyClientId].mRotation.y;
		p.x = mPlayers[mMyClientId].mPos.x;
		p.y = mPlayers[mMyClientId].mPos.y;
		p.z = mPlayers[mMyClientId].mPos.z;
		SendPacket(&p);
	}
}

void DX::Draw(const GameTimer& gt)
{
	float x = mPlayers[mMyClientId].mPos.x - mRadius * cosf(mTheta) * sinf(XM_PI / 2.0f - mPhi);
	float z = mPlayers[mMyClientId].mPos.z - mRadius * sinf(mTheta) * sinf(XM_PI / 2.0f - mPhi);
	float y = mPlayers[mMyClientId].mPos.y + mChaHeight + mRadius * cosf(XM_PI / 2.0f - mPhi);
	XMVECTOR pos = XMVectorSet(x, y, z, 0.0f);
	XMVECTOR target = XMVectorSet(mPlayers[mMyClientId].mPos.x, mPlayers[mMyClientId].mPos.y + mChaHeight, mPlayers[mMyClientId].mPos.z, 0.0f); //
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	mDirectCmdListAlloc->Reset();
	mCmdList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get());
	mCmdList->RSSetViewports(1, &mScreenViewport);
	mCmdList->RSSetScissorRects(1, &mScissorRect);
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	mCmdList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescHeap.Get() };
	mCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCmdList->SetGraphicsRootSignature(mRootSignature.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	for (int i = 0; i < PLAYERMAX + OBJMAX; i++)
	{
		if (mPlayers[i].mOn == true && i < PLAYERMAX)
		{
			DrawObj(mPlayers, mPlayers[i].mScale, mPlayers[i].mRotation, mPlayers[i].mPos, view, proj, mPlayers[i].mTexMat, tex, i, mPlayers[i].mWorld, mObjConstantBuffer);
		}
		else if (i >= PLAYERMAX && i < PLAYERMAX + OBJMAX)
		{
			DrawObj(mObjs, mObjs[i - PLAYERMAX].mScale, mObjs[i - PLAYERMAX].mRotation, mObjs[i - PLAYERMAX].mPos, view, proj, mObjs[i - PLAYERMAX].mTexMat, tex, i - PLAYERMAX, mObjs[i - PLAYERMAX].mWorld, mObjConstantBuffer2);
		}
		tex.Offset(1, mCbvSrvDescSize);
	}

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	mCmdList->Close();

	ID3D12CommandList* cmdsLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void DX::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mHMainWnd);
}

void DX::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void DX::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		if (x - mLastMousePos.x > 0.0f)
		{
			mTheta -= 0.05f;
		}
		else if (x - mLastMousePos.x < 0.0f)
		{
			mTheta += 0.05f;
		}

		if (y - mLastMousePos.y > 0.0f)
		{
			mPhi += 0.05f;
		}
		else if (y - mLastMousePos.y < 0.0f)
		{
			mPhi -= 0.05f;
		}

		float yLimitValue = XM_PI / 6.0f;

		if (mPhi >= XM_PI / 2.0f - yLimitValue)
		{
			mPhi = XM_PI / 2.0f - yLimitValue;
		}
		if (mPhi <= -XM_PI / 2.0f + XM_PI / 3.0f)
		{
			mPhi = -XM_PI / 2.0f + XM_PI / 3.0f;
		}
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void DX::BuildDescriptorHeaps(int count)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = count;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescHeap));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mPlayers[0].mTexture->Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mPlayers[0].mTexture->Resource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	mD3dDevice->CreateShaderResourceView(mPlayers[0].mTexture->Resource.Get(), &srvDesc, hDescriptor);

	for (int i = 1; i < count; i++)
	{
		if (i < PLAYERMAX)
		{
			hDescriptor.Offset(1, mCbvSrvDescSize);
			srvDesc.Format = mPlayers[i].mTexture->Resource->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = mPlayers[i].mTexture->Resource->GetDesc().MipLevels;
			mD3dDevice->CreateShaderResourceView(mPlayers[i].mTexture->Resource.Get(), &srvDesc, hDescriptor);
		}
		else if (i < PLAYERMAX + OBJMAX && i >= PLAYERMAX)
		{
			hDescriptor.Offset(1, mCbvSrvDescSize);
			srvDesc.Format = mObjs[i - PLAYERMAX].mTexture->Resource->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = mObjs[i - PLAYERMAX].mTexture->Resource->GetDesc().MipLevels;
			mD3dDevice->CreateShaderResourceView(mObjs[i - PLAYERMAX].mTexture->Resource.Get(), &srvDesc, hDescriptor);
		}
	}

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
}

void DX::BuildConstantBuffers()
{
	mObjConstantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(mD3dDevice.Get(), PLAYERMAX, true);
	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjConstantBuffer->Resource()->GetGPUVirtualAddress();

	for (int i = 0; i < PLAYERMAX; i++)
	{
		mPlayers[i].mObjCBIndex = i;
	}

	cbAddress += mPlayers[0].mObjCBIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = CalcConstantBufferByteSize(sizeof(ObjectConstants)) * PLAYERMAX;

	mD3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX::BuildConstantBuffers2()
{
	mObjConstantBuffer2 = std::make_unique<UploadBuffer<ObjectConstants>>(mD3dDevice.Get(), OBJMAX, true);
	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjConstantBuffer2->Resource()->GetGPUVirtualAddress();

	for (int i = 0; i < OBJMAX; i++)
	{
		mObjs[i].mObjCBIndex = i;
	}

	cbAddress += mObjs[0].mObjCBIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = CalcConstantBufferByteSize(sizeof(ObjectConstants)) * OBJMAX;

	mD3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	mD3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
}

void DX::BuildShadersAndInputLayout()
{
	UINT compileFlags = 0;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	D3DCompileFromFile(L"Shaders\\color.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", compileFlags, 0, &byteCode, &errors);
	mVsByteCode = byteCode;

	UINT compileFlags2 = 0;
	ComPtr<ID3DBlob> byteCode2 = nullptr;
	ComPtr<ID3DBlob> errors2;
	D3DCompileFromFile(L"Shaders\\color.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", compileFlags2, 0, &byteCode2, &errors2);
	mPsByteCode = byteCode2;

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void DX::BuildBoxGeometry(MyObj* objs, MyObj myObj, int index)
{
	FbxManager* lSdkManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);
	FbxString lPath = FbxGetApplicationDirectory();
	lSdkManager->LoadPluginsDirectory(lPath.Buffer());
	FbxScene* lScene = FbxScene::Create(lSdkManager, myObj.mSceneName);
	bool lResult;

	lResult = LoadScene(lSdkManager, lScene, myObj.mFbxFilePath);
	FbxNode* lNode = lScene->GetRootNode();

	FbxGeometryConverter geoConverter(lSdkManager);
	geoConverter.Triangulate(lScene, true);

	DisplayContent(objs, lNode, index);
}

void DX::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mVsByteCode->GetBufferPointer()), mVsByteCode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mPsByteCode->GetBufferPointer()), mPsByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
}

void DX::LoadTextures(MyObj* mObjs, MyObj myObj, int index)
{
	mObjs[index].mTexture = std::make_unique<Texture>();
	mObjs[index].mTexture->Filename = myObj.mTexFilePath;
	DirectX::CreateDDSTextureFromFile12(mD3dDevice.Get(), mCmdList.Get(), mObjs[index].mTexture->Filename.c_str(), mObjs[index].mTexture->Resource, mObjs[index].mTexture->UploadHeap);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> DX::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP); // shaderRegister 0
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.0f, 8);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

bool DX::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int i, lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing client_main filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		FBXSDK_printf("\n");

		for (i = 0; i < lAnimStackCount; i++)
		{
			FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

			FBXSDK_printf("    Animation Stack %d\n", i);
			FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
			FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

			// Change the value of the import name if the animation stack should be imported 
			// under client_main different name.
			FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

			// Set the value of the import state to false if the animation stack should be not
			// be imported. 
			FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
			FBXSDK_printf("\n");
		}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}

void DX::DisplayContent(MyObj* objs, FbxNode* pNode, int index)
{
	FbxNodeAttribute::EType lAttributeType;
	int j;

	if (pNode->GetNodeAttribute() == NULL)
	{
		FBXSDK_printf("NULL Node Attribute\n\n");
	}
	else
	{
		lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

		switch (lAttributeType)
		{
		default:
			break;

		case FbxNodeAttribute::eMesh:
			FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();
			DisplayPolygons(objs, lMesh, index);
			break;
		}
	}

	for (j = 0; j < pNode->GetChildCount(); j++)
	{
		DisplayContent(objs, pNode->GetChild(j), index);
	}
}

void DX::DisplayPolygons(MyObj* objs, FbxMesh* pMesh, int index)
{
	int count = pMesh->GetControlPointsCount();
	vector<Vertex> vertices;
	vertices.resize(count);

	FbxVector4* controlPoints = pMesh->GetControlPoints();

	for (int i = 0; i < count; i++)
	{
		vertices[i].Pos.x = static_cast<float>(controlPoints[i].mData[0]);
		vertices[i].Pos.y = static_cast<float>(controlPoints[i].mData[2]);
		vertices[i].Pos.z = static_cast<float>(controlPoints[i].mData[1]);
	}

	int triCount = pMesh->GetPolygonCount();

	vector<int> indices;
	indices.resize(triCount * 3);

	FbxGeometryElementMaterial* geometryElementMaterial = pMesh->GetElementMaterial();

	const int polygonSize = pMesh->GetPolygonSize(0);

	UINT arrIdx[3];
	UINT vertexCounter = 0;

	for (int i = 0; i < triCount; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			int controlPointIndex = pMesh->GetPolygonVertex(i, j);
			arrIdx[j] = controlPointIndex;

			vertices[controlPointIndex].Normal.x = 0.5f;
			vertices[controlPointIndex].Normal.y = 0.5f;
			vertices[controlPointIndex].Normal.z = 0.5f;

			FbxVector2 uv = pMesh->GetElementUV()->GetDirectArray().GetAt(pMesh->GetTextureUVIndex(i, j));
			vertices[controlPointIndex].TexC.x = static_cast<float>(uv.mData[0]);
			vertices[controlPointIndex].TexC.y = 1.f - static_cast<float>(uv.mData[1]);

			vertexCounter++;
		}

		indices.push_back(arrIdx[0]);
		indices.push_back(arrIdx[2]);
		indices.push_back(arrIdx[1]);
	}

	const UINT vertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT indexBufferByteSize = (UINT)indices.size() * sizeof(uint32_t);

	D3DCreateBlob(vertexBufferByteSize, &objs[index].mMeshGeo->VertexBufferCPU);
	CopyMemory(objs[index].mMeshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);
	D3DCreateBlob(indexBufferByteSize, &objs[index].mMeshGeo->IndexBufferCPU);
	CopyMemory(objs[index].mMeshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

	ComPtr<ID3D12Resource> defaultBuffer;
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf()));
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(objs[index].mMeshGeo->VertexBufferUploader.GetAddressOf()));
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = vertices.data();
	subResourceData.RowPitch = vertexBufferByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCmdList.Get(), defaultBuffer.Get(), objs[index].mMeshGeo->VertexBufferUploader.Get(), 0, 0, 1, &subResourceData);
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	objs[index].mMeshGeo->VertexBufferGPU = defaultBuffer;

	ComPtr<ID3D12Resource> defaultBuffer2;
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer2.GetAddressOf()));
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(objs[index].mMeshGeo->IndexBufferUploader.GetAddressOf()));
	D3D12_SUBRESOURCE_DATA subResourceData2 = {};
	subResourceData2.pData = indices.data();
	subResourceData2.RowPitch = indexBufferByteSize;
	subResourceData2.SlicePitch = subResourceData2.RowPitch;
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer2.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCmdList.Get(), defaultBuffer2.Get(), objs[index].mMeshGeo->IndexBufferUploader.Get(), 0, 0, 1, &subResourceData2);
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	objs[index].mMeshGeo->IndexBufferGPU = defaultBuffer2;

	objs[index].mMeshGeo->VertexByteStride = sizeof(Vertex);
	objs[index].mMeshGeo->VertexBufferByteSize = vertexBufferByteSize;
	objs[index].mMeshGeo->IndexFormat = DXGI_FORMAT_R32_UINT;
	objs[index].mMeshGeo->IndexBufferByteSize = indexBufferByteSize;

	objs[index].mMeshGeo->IndexCount = (UINT)indices.size();
	objs[index].mMeshGeo->StartIndexLocation = 0;
	objs[index].mMeshGeo->BaseVertexLocation = 0;
}

void DX::DrawObj(MyObj* objs, XMFLOAT3 scale, XMFLOAT3 rotate, XMFLOAT3 translation, XMMATRIX view, XMMATRIX proj, XMMATRIX texTransform, CD3DX12_GPU_DESCRIPTOR_HANDLE& tex, int index, XMFLOAT4X4& mWorld, unique_ptr<UploadBuffer<ObjectConstants>>& mObjConstantBuffer)
{
	XMStoreFloat4x4(&mWorld, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixRotationZ(rotate.z) * XMMatrixRotationY(rotate.y) * XMMatrixRotationX(rotate.x) * XMMatrixTranslation(translation.x, translation.y, translation.z));
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX wvp = world * view * proj;
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(wvp));
	XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
	mObjConstantBuffer->CopyData(index, objConstants);

	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress = mObjConstantBuffer->Resource()->GetGPUVirtualAddress() + index * objCBByteSize;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = objs[index].mMeshGeo->VertexBufferGPU->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = objs[index].mMeshGeo->VertexByteStride;
	vertexBufferView.SizeInBytes = objs[index].mMeshGeo->VertexBufferByteSize;
	mCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);

	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	indexBufferView.BufferLocation = objs[index].mMeshGeo->IndexBufferGPU->GetGPUVirtualAddress();
	indexBufferView.Format = objs[index].mMeshGeo->IndexFormat;
	indexBufferView.SizeInBytes = objs[index].mMeshGeo->IndexBufferByteSize;
	mCmdList->IASetIndexBuffer(&indexBufferView);

	mCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCmdList->SetGraphicsRootDescriptorTable(0, tex);
	mCmdList->SetGraphicsRootConstantBufferView(1, constantBufferAddress);
	mCmdList->DrawIndexedInstanced(objs[index].mMeshGeo->IndexCount, 1, 0, 0, 0);
}

MeshData DX::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	MeshData meshData;

	uint32_t vertexCount = m * n;
	uint32_t faceCount = (m - 1) * (n - 1) * 2;

	float halfWidth = 0.5f * width;
	float halfDepth = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.mVertices.resize(vertexCount);
	for (uint32_t i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.mVertices[i * n + j].Pos = XMFLOAT3(x, 0.0f, z);
			meshData.mVertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

			meshData.mVertices[i * n + j].TexC.x = j * du;
			meshData.mVertices[i * n + j].TexC.y = i * dv;
		}
	}

	meshData.mIndices32.resize(faceCount * 3);

	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i)
	{
		for (uint32_t j = 0; j < n - 1; ++j)
		{
			meshData.mIndices32[k] = i * n + j;
			meshData.mIndices32[k + 1] = i * n + j + 1;
			meshData.mIndices32[k + 2] = (i + 1) * n + j;

			meshData.mIndices32[k + 3] = (i + 1) * n + j;
			meshData.mIndices32[k + 4] = i * n + j + 1;
			meshData.mIndices32[k + 5] = (i + 1) * n + j + 1;

			k += 6;
		}
	}

	return meshData;
}

void DX::BuildLandGeometry(int index)
{
	MeshData grid = CreateGrid(160.0f, 160.0f, 50, 50);

	vector<Vertex> vertices(grid.mVertices.size());
	for (size_t i = 0; i < grid.mVertices.size(); ++i)
	{
		auto& p = grid.mVertices[i].Pos;
		vertices[i].Pos = p;
		vertices[i].Pos.y = 0.3f * (p.z * sinf(0.1f * p.x) + p.x * cosf(0.1f * p.z));

		vertices[i].Normal = grid.mVertices[i].Normal;
		vertices[i].TexC = grid.mVertices[i].TexC;
	}

	vector<uint32_t> indices = grid.GetIndices16();

	const UINT vertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT indexBufferByteSize = (UINT)indices.size() * sizeof(uint32_t);

	D3DCreateBlob(vertexBufferByteSize, &mObjs[index].mMeshGeo->VertexBufferCPU);
	CopyMemory(mObjs[index].mMeshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);
	D3DCreateBlob(indexBufferByteSize, &mObjs[index].mMeshGeo->IndexBufferCPU);
	CopyMemory(mObjs[index].mMeshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

	ComPtr<ID3D12Resource> defaultBuffer;
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf()));
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(mObjs[index].mMeshGeo->VertexBufferUploader.GetAddressOf()));
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = vertices.data();
	subResourceData.RowPitch = vertexBufferByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCmdList.Get(), defaultBuffer.Get(), mObjs[index].mMeshGeo->VertexBufferUploader.Get(), 0, 0, 1, &subResourceData);
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	mObjs[index].mMeshGeo->VertexBufferGPU = defaultBuffer;

	ComPtr<ID3D12Resource> defaultBuffer2;
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferByteSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer2.GetAddressOf()));
	mD3dDevice.Get()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferByteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(mObjs[index].mMeshGeo->IndexBufferUploader.GetAddressOf()));
	D3D12_SUBRESOURCE_DATA subResourceData2 = {};
	subResourceData2.pData = indices.data();
	subResourceData2.RowPitch = indexBufferByteSize;
	subResourceData2.SlicePitch = subResourceData2.RowPitch;
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer2.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCmdList.Get(), defaultBuffer2.Get(), mObjs[index].mMeshGeo->IndexBufferUploader.Get(), 0, 0, 1, &subResourceData2);
	mCmdList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	mObjs[index].mMeshGeo->IndexBufferGPU = defaultBuffer2;

	mObjs[index].mMeshGeo->VertexByteStride = sizeof(Vertex);
	mObjs[index].mMeshGeo->VertexBufferByteSize = vertexBufferByteSize;
	mObjs[index].mMeshGeo->IndexFormat = DXGI_FORMAT_R32_UINT;
	mObjs[index].mMeshGeo->IndexBufferByteSize = indexBufferByteSize;

	mObjs[index].mMeshGeo->IndexCount = (UINT)indices.size();
	mObjs[index].mMeshGeo->StartIndexLocation = 0;
	mObjs[index].mMeshGeo->BaseVertexLocation = 0;
}

void DX::Input()
{
	HWND hwnd = ::GetActiveWindow();

	for (UINT key = 0; key < 255; key++)
	{
		if (::GetAsyncKeyState(key) & 0x8000)
		{
			if (mKeyStates[key] == 1 || mKeyStates[key] == 2)
				mKeyStates[key] = 1;
			else
				mKeyStates[key] = 2;
		}
		else
		{
			int& state = mKeyStates[key];

			if (mKeyStates[key] == 1 || mKeyStates[key] == 2)
				mKeyStates[key] = 3;
			else
				mKeyStates[key] = 0;
		}
	}
}

void DX::ConnectServer()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = mSocket.connect("127.0.0.1", PORT_NUM);
	mSocket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}

	CS_LOGIN_PACKET p;
	p.size = sizeof(CS_LOGIN_PACKET);
	p.type = CS_LOGIN;
	strcpy_s(p.name, "a");
	SendPacket(&p);
}

void DX::ReceiveServer()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = mSocket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) ProcessData(net_buf, received);
}

void DX::ProcessData(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void DX::ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_OK:
	{
		SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
		mMyClientId = packet->id;
		mPlayers[mMyClientId].mOn = true;
		mPlayers[mMyClientId].mPos.x = packet->x;
		mPlayers[mMyClientId].mPos.y = packet->y;
		mPlayers[mMyClientId].mPos.z = packet->z;
		
		break;
	}
	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;
		if (id < PLAYERMAX) {
			mPlayers[id].mOn = true;
			mPlayers[id].mPos.x = my_packet->x;
			mPlayers[id].mPos.y = my_packet->y;
			mPlayers[id].mPos.z = my_packet->z;
			mPlayers[id].mClientId = id;
		}

		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;
		mPlayers[id].mPos.x = my_packet->x;
		mPlayers[id].mPos.y = my_packet->y;
		mPlayers[id].mPos.z = my_packet->z;
		mPlayers[id].mRotation.y = my_packet->degree;
		printf_s("x : %f, y : %f, z : %f\n", mPlayers[id].mPos.x, mPlayers[id].mPos.y, mPlayers[id].mPos.z);
		break;
	}
	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;
		mPlayers[id].mOn = false;
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void DX::SendPacket(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	mSocket.send(packet, p[0], sent);
}
