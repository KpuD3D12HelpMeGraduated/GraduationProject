#pragma once
#include "Util.h"
class Camera
{
public:
	XMFLOAT4X4 mWorld = Identity4x4(); //월드 변환 행렬
	XMFLOAT4X4 mView = Identity4x4(); //뷰 변환 행렬
	XMFLOAT4X4 mProj = Identity4x4(); //투영 변환 행렬

	//투영 변환
	void TransformProjection(WindowInfo windowInfo)
	{
		XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * 3.14, static_cast<float>(windowInfo.ClientWidth) / windowInfo.ClientHeight, 1.0f, 1000.0f);
		XMStoreFloat4x4(&mProj, P);
	}
};