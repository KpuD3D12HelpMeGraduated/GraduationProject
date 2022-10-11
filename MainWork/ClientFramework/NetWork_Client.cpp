#include "NetWork_Client.h"

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);

	exit(1);
}

void NetWork_Client::cmd_list_push()
{


}



NetWork_Client::NetWork_Client() {
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		ErrorHandling((char*)"WSAStartup() error!");
	}

	memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(PORT_NUM);
	_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(_socket, reinterpret_cast<sockaddr*>(&_addr), sizeof(_addr));
}