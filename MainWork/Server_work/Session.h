#pragma once
#include <mutex>
#include <thread>
#include <iostream>
#include "protocol.h"
#include "Over_EXP.h"

enum SESSION_STATE { ST_FREE, ST_ACCEPTED, ST_INGAME };

class SESSION {
protected:
	OVER_EXP _recv_over;
public:
	SESSION_STATE _s_state;
	int _id;
	SOCKET _socket;
	float	x, y, z;
	float	degree;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	std::mutex	_sl;
public:
	SESSION();
	~SESSION();
	void do_recv();
	void do_send(void* packet);
	void send_login_ok_packet(int c_id, float x, float y, float z, float degree);
	void send_move_packet(int c_id, float x, float y, float z, float degree);
	void send_add_object(int c_id, float x, float y, float z, float degree, char* name);
	void send_remove_object(int c_id);
};