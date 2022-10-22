#include "Session.h"

SESSION::SESSION()
{
	_id = -1;
	_socket = 0;
	x = 0;
	y = 0;
	z = 0;
	degree = 0;
	_name[0] = 0;
	_s_state = ST_FREE;
	_prev_remain = 0;
	chn = true;
}

SESSION::~SESSION()
{

}

void SESSION::do_recv()
{
	DWORD recv_flag = 0;
	memset(&_recv_over._over, 0, sizeof(_recv_over._over));
	_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
	_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
	WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
}

void SESSION::do_send(void* packet)
{
	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
}

void SESSION::send_login_ok_packet(int c_id, float x, float y, float z, float degree)
{
	SC_LOGIN_OK_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_LOGIN_OK_PACKET);
	p.type = SC_LOGIN_OK;
	p.x = x;
	p.y = y;
	p.z = z;
	do_send(&p);
}

void SESSION::send_move_packet(int c_id, float x, float y, float z, float degree)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = x;
	p.y = y;
	p.z = z;
	do_send(&p);
}

void SESSION::send_add_object(int c_id, float x, float y, float z, float degree, char* name)
{
	SC_ADD_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_ADD_OBJECT_PACKET);
	p.type = SC_ADD_OBJECT;
	p.x = x;
	p.y = y;
	p.z = z;
	strcpy_s(p.name, name);
	do_send(&p);
}

void SESSION::send_remove_object(int c_id)
{
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	p.type = SC_REMOVE_OBJECT;
	do_send(&p);
}