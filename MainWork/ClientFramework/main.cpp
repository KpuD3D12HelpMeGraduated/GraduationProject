#include "Client.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
	Client client;
	//Ŭ���̾�Ʈ �ʱ�ȭ
	client.Init(hInst, nCmdShow);
	//�� �����Ӹ��� ������Ʈ
	client.Update();
}

