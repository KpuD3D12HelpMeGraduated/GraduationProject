#include "Client.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
	Client client;
	//클라이언트 초기화
	client.Init(hInst, nCmdShow);
	//매 프레임마다 업데이트
	client.Update();
}

