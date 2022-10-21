#pragma once

#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <Windows.h>
#include "protocol.h"

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE};

class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int target_id;
	OVER_EXP();
	OVER_EXP(char* packet);
};