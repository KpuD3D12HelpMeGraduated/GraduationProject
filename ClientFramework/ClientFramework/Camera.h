#pragma once
#include "Util.h"

class Camera
{
public:
	XMFLOAT4X4 mProj = Identity4x4(); //투영 변환 행렬

	//투영 변환
	void TransformProjection(WindowInfo windowInfo);
};