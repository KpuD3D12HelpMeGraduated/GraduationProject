#pragma once

#include "Util/MathHelper.h"
#include "Util/UploadBuffer.h"
#include "Util/d3dUtil.h"
#include "Util/GameTimer.h"
#include <WindowsX.h>
#include <fbxsdk.h>
#include <vector>
#include <array>

#pragma comment(lib, "libfbxsdk-md.lib")

#define SFML_STATIC 1

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

#define CHARACTERINDEX 0
#define PLAYERMAX 5
#define OBJMAX 4

struct MeshData
{
	vector<uint32_t>& GetIndices16()
	{
		if (mIndices16.empty())
		{
			mIndices16.resize(mIndices32.size());
			for (size_t i = 0; i < mIndices32.size(); ++i)
				mIndices16[i] = static_cast<uint32_t>(mIndices32[i]);
		}

		return mIndices16;
	}

	vector<Vertex> mVertices;
	vector<uint32_t> mIndices32;

private:
	vector<uint32_t> mIndices16;
};

class MyObj
{
public:
	MyObj() {};
	MyObj(XMFLOAT3 scale, XMFLOAT3 rotation, XMFLOAT3 position, XMFLOAT3 texScale, const wchar_t* texFilePath, const char* fbxFilePath)
	{
		mMeshGeo = make_unique<MeshGeometry>();
		mScale = scale;
		mRotation = rotation;
		mPos = position;
		XMStoreFloat4x4(&mTexTransform, XMMatrixScaling(texScale.x, texScale.y, texScale.z));
		mTexMat = XMLoadFloat4x4(&mTexTransform);
		mTexFilePath = texFilePath;
		mFbxFilePath = fbxFilePath;
	}

	void Init(XMFLOAT3 scale, XMFLOAT3 rotataion, XMFLOAT3 pos, XMFLOAT3 texScale, const wchar_t* texFilePath, const char* fbxFilePath)
	{
		mMeshGeo = make_unique<MeshGeometry>();
		mScale = scale;
		mRotation = rotataion;
		mPos = pos;
		XMStoreFloat4x4(&mTexTransform, XMMatrixScaling(texScale.x, texScale.y, texScale.z));
		mTexMat = XMLoadFloat4x4(&mTexTransform);
		mTexFilePath = texFilePath;
		mFbxFilePath = fbxFilePath;
	}

	shared_ptr<MeshGeometry> mMeshGeo;
	int mObjCBIndex;
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	shared_ptr<Texture> mTexture;
	XMFLOAT4X4 mTexTransform = MathHelper::Identity4x4();
	XMMATRIX mTexMat;

	bool mOn = false;
	XMFLOAT3 mScale;
	XMFLOAT3 mRotation;
	XMFLOAT3 mPos;
	int mClientId;

	const wchar_t* mTexFilePath;

	const char* mSceneName = "My Scene";
	const char* mFbxFilePath;
};

class DX
{
public:
	DX(HINSTANCE hInstance);
	~DX();

	//윈도우
	bool InitWinApi();
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	int Run();

	//다렉
	bool InitDirect3D();
	static DX* GetApp();
	void Set4xMsaaState(bool value);
	array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void DrawObj(MyObj* objs, XMFLOAT3 scale, XMFLOAT3 rotate, XMFLOAT3 translation, XMMATRIX view, XMMATRIX proj, XMMATRIX texTransform, CD3DX12_GPU_DESCRIPTOR_HANDLE& tex, int index, XMFLOAT4X4& mWorld, unique_ptr<UploadBuffer<ObjectConstants>>& mObjConstantBuffer);
	void Input();

	//fbx
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
	void DisplayContent(MyObj* objs, FbxNode* pNode, int index);
	void DisplayPolygons(MyObj* objs, FbxMesh* pMesh, int index);

	//통신
	void ConnectServer();
	void ReceiveServer();
	void ProcessPacket(char* ptr);
	void ProcessData(char* net_buf, size_t io_byte);
	void SendPacket(void* packet);

private:
	//다렉
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();
	void FlushCommandQueue();
	void LoadTextures(MyObj* mObjs, MyObj myObj, int index);
	void BuildDescriptorHeaps(int count);
	void BuildConstantBuffers();
	void BuildConstantBuffers2();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry(MyObj* objs, MyObj myObj, int index);
	void BuildPSO();
	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void Update(const GameTimer& gt);
	void Draw(const GameTimer& gt);
	void OnResize();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void CalculateFrameStats();

	MeshData CreateGrid(float width, float depth, uint32_t m, uint32_t n);
	void BuildLandGeometry(int index);

private:
	//윈도우
	wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE mD3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
	HINSTANCE mHAppInst = nullptr;
	HWND mHMainWnd = nullptr;
	bool mAppPaused = false;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mResizing = false;
	bool mFullscreenState = false;

	//다렉
	static DX* mApp;
	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D12Device> mD3dDevice;

	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	ComPtr<ID3D12CommandQueue> mCmdQueue;
	ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	ComPtr<ID3D12GraphicsCommandList> mCmdList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescSize = 0;
	UINT mDsvDescSize = 0;
	UINT mCbvSrvUavDescSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	unique_ptr<UploadBuffer<ObjectConstants>> mObjConstantBuffer = nullptr;
	unique_ptr<UploadBuffer<ObjectConstants>> mObjConstantBuffer2 = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvDescHeap = nullptr;

	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	ComPtr<ID3DBlob> mVsByteCode = nullptr;
	ComPtr<ID3DBlob> mPsByteCode = nullptr;

	UINT mCbvSrvDescSize = 0;

	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	bool mIsTerrain = false;

	float mTheta = XM_PI / 2.0f;
	float mPhi = 0.0f;
	float mRadius = 3.0f;
	float mChaHeight = 1.0f;
	POINT mLastMousePos;

	vector<int> mKeyStates;

	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	GameTimer mTimer;

	//fbx
	FbxManager* mFbxManager = nullptr;
	FbxScene* mFbxScene = nullptr;
	FbxNode* mFbxNode = nullptr;
	FbxMesh* mFbxMesh = nullptr;
	FbxIOSettings* mFbxIOSet = nullptr;
	FbxVector4* mPos = nullptr;

	//통신
	sf::TcpSocket mSocket;
	int mMyClientId;

public:
	MyObj mPlayers[PLAYERMAX];
	MyObj mObjs[4];
};

#include <iostream>
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;
constexpr int MAX_USER = 10;

// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;

constexpr char SC_LOGIN_OK = 11;
constexpr char SC_ADD_OBJECT = 12;
constexpr char SC_REMOVE_OBJECT = 13;
constexpr char SC_MOVE_OBJECT = 14;

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned char size;
	char	type;
	float	degree;
	float	x;
	float	y;
	float	z;
	unsigned  client_time;
};

struct SC_LOGIN_OK_PACKET {
	unsigned char size;
	char	type;
	int	id;
	float	x, y, z;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int		id;
	float	x, y, z;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int	id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned char size;
	char	type;
	int	id;
	float	x, y, z, degree;
	unsigned int client_time;
};

#pragma pack (pop)

