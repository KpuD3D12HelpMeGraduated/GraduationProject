#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <iostream>
#include <winsock2.h> // 윈속2 메인 헤더
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <windows.h>

#include "..\Server_work\protocol.h"


//서버정보
#define LOCAL_IP "127.0.0.1"
#define HAEIN_SERVER "haein0303.iptime.org"


#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

using namespace std;

void ErrorHandling(char* message);


class NetWork_Client
{

private:

	HANDLE _h_iocp;
	SOCKET _socket;

	SOCKADDR_IN _addr;      

	// 윈속 초기화
	WSADATA wsa;
	

	
public:
	//싱글톤
	NetWork_Client* instance() {
		static NetWork_Client _instance;
		return &_instance;
	}

	//기본생성자
	NetWork_Client();



	//커멘드리스트에 푸쉬합니다
	void cmd_list_push();




};

