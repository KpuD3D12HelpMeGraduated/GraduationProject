#include "Camera.h"

void Camera::TransformProjection(WindowInfo windowInfo)
{
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * 3.14, static_cast<float>(windowInfo.ClientWidth) / windowInfo.ClientHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}