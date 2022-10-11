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
#define PLAYERMAX 1
#define OBJMAX 4

struct Keyframe
{
	Keyframe() : TimePos(0.0f), Translation(0.0f, 0.0f, 0.0f), Scale(1.0f, 1.0f, 1.0f), RotationQuat(0.0f, 0.0f, 0.0f, 1.0f) { }

	float TimePos;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT3 Scale;
	DirectX::XMFLOAT4 RotationQuat;
};

struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

	void Interpolate(float t, DirectX::XMFLOAT4X4& M)const;

	std::vector<Keyframe> Keyframes;
};

struct AnimationClip
{
	float GetClipStartTime()const;
	float GetClipEndTime()const;

	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const;

	std::vector<BoneAnimation> BoneAnimations;
};

class SkinnedData
{
public:

	UINT BoneCount()const;

	float GetClipStartTime(const std::string& clipName)const;
	float GetClipEndTime(const std::string& clipName)const;

	// In a real project, you'd want to cache the result if there was a chance
	// that you were calling this several times with the same clipName at 
	// the same timePos.
	void GetFinalTransforms(const std::string& clipName, float timePos,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms)const;

	// Gives parentIndex of ith bone.
	std::vector<int> mBoneHierarchy;

	std::vector<DirectX::XMFLOAT4X4> mBoneOffsets;

	std::unordered_map<std::string, AnimationClip> mAnimations;
};

struct SkinnedModelInstance
{
	SkinnedData* SkinnedInfo = nullptr;
	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
	std::string ClipName;
	float TimePos = 0.0f;

	std::vector<int> mBoneHierarchy;

	std::vector<DirectX::XMFLOAT4X4> mBoneOffsets;

	std::vector<AnimationClip> mAnimations;

	// Called every frame and increments the time position, interpolates the 
	// animations for each bone based on the current animation clip, and 
	// generates the final transforms which are ultimately set to the effect
	// for processing in the vertex shader.
	void UpdateSkinnedAnimation(float dt)
	{
		TimePos += dt;

		// Loop animation
		if (TimePos > SkinnedInfo->GetClipEndTime(ClipName))
			TimePos = 0.0f;

		// Compute the final transforms for this time position.
		SkinnedInfo->GetFinalTransforms(ClipName, TimePos, FinalTransforms);
	}
};

struct SkinnedConstants
{
	DirectX::XMFLOAT4X4 BoneTransforms[96];
};

struct FbxKeyFrameInfo
{
	FbxAMatrix  matTransform;
	double		time;
};

struct FbxAnimClipInfo
{
	wstring			name;
	FbxTime			startTime;
	FbxTime			endTime;
	FbxTime::EMode	mode;
	vector<vector<FbxKeyFrameInfo>>	keyFrames;
};

struct BoneWeight
{
	using Pair = pair<int, double>;
	vector<Pair> boneWeights;

	void AddWeights(UINT index, double weight)
	{
		if (weight <= 0.f)
			return;

		auto findIt = std::find_if(boneWeights.begin(), boneWeights.end(),
			[=](const Pair& p) { return p.second < weight; });

		if (findIt != boneWeights.end())
			boneWeights.insert(findIt, Pair(index, weight));
		else
			boneWeights.push_back(Pair(index, weight));

		if (boneWeights.size() > 4)
			boneWeights.pop_back();
	}

	void Normalize()
	{
		double sum = 0.f;
		std::for_each(boneWeights.begin(), boneWeights.end(), [&](Pair& p) { sum += p.second; });
		std::for_each(boneWeights.begin(), boneWeights.end(), [=](Pair& p) { p.second = p.second / sum; });
	}
};

struct FbxBoneInfo
{
	wstring					boneName;
	int					  parentIndex;
	FbxAMatrix				matOffset;
};

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
	void LoadBones(FbxNode* node, int idx, int parentIdx);
	void LoadAnimationData(FbxMesh* mesh);
	int FindBoneIndex(string name);
	FbxAMatrix GetTransform(FbxNode* node);
	void LoadBoneWeight(FbxCluster* cluster, int boneIdx);
	void LoadOffsetMatrix(FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int boneIdx);
	void LoadKeyframe(int animIndex, FbxNode* node, FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int boneIdx);
	void LoadAnimationInfo();
	void UpdateSkinnedCBs(const GameTimer& gt);

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
	FbxScene* lScene;
	FbxNode* mFbxNode = nullptr;
	FbxMesh* mFbxMesh = nullptr;
	FbxIOSettings* mFbxIOSet = nullptr;
	FbxVector4* mPos = nullptr;
	vector<shared_ptr<FbxBoneInfo>>		_bones;
	vector<BoneWeight>					_boneWeights; // 뼈 가중치
	FbxArray<FbxString*>				_animNames;
	vector<shared_ptr<FbxAnimClipInfo>>	_animClips;
	std::unique_ptr<SkinnedModelInstance> mSkinnedModelInst;
	//SkinnedData mSkinnedInfo;
	XMFLOAT4X4 GetMatrix(FbxAMatrix& matrix);

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

