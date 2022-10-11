#pragma once
//라이브러리
#include <Windows.h>
#include <tchar.h>

#include <dxgi1_4.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXColors.h>
#include <vector>

#include <string>
#include <filesystem>

#include "DirectXTex.h"
#include "DirectXTex.inl"

#ifdef _DEBUG
#pragma comment(lib, "..\\DirectXTex_debug.lib")
#else
#pragma comment(lib, "..\\DirectXTex.lib")
#endif

#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "D3D12.lib")
#pragma comment (lib, "dxgi.lib")

//네임스페이스
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

#define SWAP_CHAIN_BUFFER_COUNT 2
#define CBV_REGISTER_COUNT 5
#define SRV_REGISTER_COUNT 5
#define REGISTER_COUNT 10


//조명정보
#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

//정점 구조체
struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT4 color;
	XMFLOAT2 uv;
};

//행렬을 단위행렬로 초기화
static XMFLOAT4X4 Identity4x4()
{
	static XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return I;
}

//상수버퍼로 넘겨줄 구조체의 형태
struct Constants
{
	XMFLOAT4X4 worldViewProj = Identity4x4();
};

//윈도우와 관련된 정보
struct WindowInfo {
	HWND hwnd;
	int ClientWidth = 600;
	int ClientHeight = 600;
};