#include "Client.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow)
{
	Client client;
	//Ŭ���̾�Ʈ �ʱ�ȭ
	client.Init(hInst, nCmdShow);
	//�� �����Ӹ��� ������Ʈ
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//�� �����Ӹ��� �׸���
			client.Update();
		}
	}

	return (int)msg.wParam;
	
}

