#include "dx.h"

using Microsoft::WRL::ComPtr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	DX theApp(hInstance);

	MyObj character({ 1.0f, 1.0f, 1.0f }, { 0.0f, XM_PI, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/AnimeCharcter.dds", "Resource/Models/AnimeCharacter.fbx");
	MyObj player1({ 0.05f, 0.05f, 0.05f }, { 0.0f, XM_PI, 0.0f }, { -5.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/RobotColor.dds", "Resource/Models/Dragon.fbx");
	MyObj player2({ 0.05f, 0.05f, 0.05f }, { 0.0f, XM_PI, 0.0f }, { 5.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/RobotColor.dds", "Resource/Models/Dragon.fbx");
	MyObj player3({ 0.05f, 0.05f, 0.05f }, { 0.0f, XM_PI, 0.0f }, { 5.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/RobotColor.dds", "Resource/Models/Dragon.fbx");
	MyObj player4({ 0.05f, 0.05f, 0.05f }, { 0.0f, XM_PI, 0.0f }, { -5.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/RobotColor.dds", "Resource/Models/Dragon.fbx");
	theApp.mPlayers[0] = character;
	theApp.mPlayers[1] = player1;
	theApp.mPlayers[2] = player2;
	theApp.mPlayers[3] = player3;
	theApp.mPlayers[4] = player4;

	MyObj terrain({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, -1.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, L"Resource/Textures/Grass.dds", "Resource/Models/Terrain.fbx");
	MyObj field({ 200.0f, 1.0f, 200.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.1f, 0.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/Grass.dds", "Resource/Models/Plane.fbx");
	MyObj obj3({ 0.005f, 0.005f, 0.005f }, { 0.0f, 0.0f, 0.0f }, { 6.0f, 0.0f, 6.0f }, { 1.f, 1.f, 1.f }, L"Resource/Textures/BakerHouse.dds", "Resource/Models/BakerHouse.fbx");
	MyObj obj4({ 0.005f, 0.005f, 0.005f }, { 0.0f, 0.0f, 0.0f }, { -6.0f, 0.0f, 6.0f }, { 1.0f, 1.0f, 1.0f }, L"Resource/Textures/BakerHouse.dds", "Resource/Models/BakerHouse.fbx");
	theApp.mObjs[0] = terrain;
	theApp.mObjs[1] = field;
	theApp.mObjs[2] = obj3;
	theApp.mObjs[3] = obj4;

	theApp.ConnectServer();

	theApp.InitWinApi();
	theApp.InitDirect3D();

	return theApp.Run();
}