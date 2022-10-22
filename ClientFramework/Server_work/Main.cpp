#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <unordered_set>
#include <windows.h>
#include <string>
#include <queue>

#include "protocol.h"
#include "Over_EXP.h"
#include "Session.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

using namespace std;

HANDLE g_h_iocp;
SOCKET g_s_socket;

array<SESSION, MAX_USER + NPC_NUM> clients;

void disconnect(int c_id);

enum EVENT_TYPE { EV_MOVE };

struct TIMER_EVENT {
	int object_id;
	EVENT_TYPE ev;
	chrono::system_clock::time_point act_time;
	int target_id;

	constexpr bool operator < (const TIMER_EVENT& _Left) const
	{
		return (act_time > _Left.act_time);
	}

};

priority_queue<TIMER_EVENT> timer_queue;
mutex timer_l;

void add_timer(int obj_id, int act_time, EVENT_TYPE e_type, int target_id)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.act_time = system_clock::now() + milliseconds(act_time);
	ev.object_id = obj_id;
	ev.ev = e_type;
	ev.target_id = target_id;

	timer_l.lock();
	timer_queue.push(ev);
	timer_l.unlock();
}

void do_timer()
{
	while (true)
	{
		this_thread::sleep_for(1ms);
		while (true) 
		{
			timer_l.lock();

			if (timer_queue.empty() == true) 
			{
				timer_l.unlock();
				break;
			}

			if (timer_queue.top().act_time > chrono::system_clock::now()) 
			{
				timer_l.unlock();
				break;
			}

			TIMER_EVENT ev = timer_queue.top();
			timer_queue.pop();
			timer_l.unlock();

			switch (ev.ev)
			{
				case EV_MOVE: 
				{
					auto ex_over = new OVER_EXP;
					ex_over->_comp_type = OP_NPC_MOVE;
					ex_over->target_id = ev.object_id;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.target_id, &ex_over->_over);
					add_timer(ev.object_id, 100, ev.ev, ev.target_id);
					break;
				}
				default:
					break;
			}
		}
	}
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		clients[i]._sl.lock();
		if (clients[i]._s_state == ST_FREE) {
			clients[i]._s_state = ST_ACCEPTED;
			clients[i]._sl.unlock();
			return i;
		}
		clients[i]._sl.unlock();
	}
	return -1;
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		clients[c_id]._sl.lock();
		if (clients[c_id]._s_state == ST_FREE) {
			clients[c_id]._sl.unlock();
			break;
		}
		if (clients[c_id]._s_state == ST_INGAME) {
			clients[c_id]._sl.unlock();
			disconnect(c_id);
			break;
		}

		strcpy_s(clients[c_id]._name, p->name);
		clients[c_id].x = 0;
		clients[c_id].y = 0;
		clients[c_id].z = 0;
		clients[c_id].degree = 0;
		clients[c_id].send_login_ok_packet(c_id, 0, 0, 0, 0);
		clients[c_id]._s_state = ST_INGAME;
		clients[c_id]._sl.unlock();

		for (int i = 0; i < MAX_USER; ++i) {
			auto& pl = clients[i];
			if (pl._id == c_id) continue;
			pl._sl.lock();
			if (ST_INGAME != pl._s_state) {
				pl._sl.unlock();
				continue;
			}
			pl.send_add_object(c_id, clients[c_id].x, clients[c_id].y, clients[c_id].z, clients[c_id].degree, clients[c_id]._name);
			clients[c_id].send_add_object(pl._id, pl.x, pl.y, pl.z, pl.degree, pl._name);
			pl._sl.unlock();
		}

		for (int i = MAX_USER; i < MAX_USER + NPC_NUM; i++) {
			clients[c_id].send_add_object(i, clients[i].x, clients[i].y, clients[i].z, clients[i].degree, clients[i]._name);
		}

		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		float x = p->x;
		float y = p->y;
		float z = p->z;
		float degree = p->degree;
		/*switch (p->direction) {
		case 0:
		{
			y--;
			break;
		}
		case 1:
		{
			y++;
			break;
		}
		case 2:
		{
			x--;
			break;
		}
		case 3:
		{
			x++;
			break;
		}
		}*/

		clients[c_id].x = x;
		clients[c_id].y = y;
		clients[c_id].z = z;
		clients[c_id].degree = degree;
		
		//clients[c_id].send_move_packet(c_id, x, y);

		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			pl._sl.lock();
			if (ST_INGAME != pl._s_state) {
				pl._sl.unlock();
				continue;
			}
			pl.send_move_packet(c_id, x, y, z, degree);
			pl._sl.unlock();
		}
		break;
	}
	}
}

void move_npc(int npc_id)
{
	float z = clients[npc_id].z;
	
	if (clients[npc_id].chn == true) z++;
	else z--;

	if (z >= 10) clients[npc_id].chn = false;
	else if (z <= -10) clients[npc_id].chn = true;

	clients[npc_id].z = z;

	for (int i = 0; i < MAX_USER;++i) {
		auto& pl = clients;
		pl[i].send_move_packet(npc_id, clients[npc_id].x, clients[npc_id].y, clients[npc_id].z, clients[npc_id].degree);
	}
}

void disconnect(int c_id)
{
	clients[c_id]._sl.lock();
	if (clients[c_id]._s_state == ST_FREE) {
		clients[c_id]._sl.unlock();
		return;
	}
	closesocket(clients[c_id]._socket);
	clients[c_id]._s_state = ST_FREE;
	clients[c_id]._sl.unlock();

	for (auto& pl : clients) {
		if (pl._id == c_id) continue;
		pl._sl.lock();
		if (pl._s_state != ST_INGAME) {
			pl._sl.unlock();
			continue;
		}
		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		pl.do_send(&p);
		pl._sl.unlock();
	}
}

void do_worker()
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		int client_id = static_cast<int>(key);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket = reinterpret_cast<SOCKET>(ex_over->_wsabuf.buf);
			int client_id = get_new_client_id();
			if (client_id != -1) {
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, client_id, 0);
				clients[client_id].do_recv();
				c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else cout << "Max user exceeded.\n";

			ZeroMemory(&ex_over->_over, sizeof(ex_over->_over));
			ex_over->_wsabuf.buf = reinterpret_cast<CHAR*>(c_socket);
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, c_socket, ex_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &ex_over->_over);
			break;
		}
		case OP_RECV: {
			if (0 == num_bytes) disconnect(client_id);
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			if (0 == num_bytes) disconnect(client_id);
			delete ex_over;
			break;

		case OP_NPC_MOVE:
		{
			move_npc(ex_over->target_id);
			delete ex_over;
			break;
		}
		}
	}
}

void initialize_npc()
{
	for (int i = MAX_USER; i < MAX_USER + NPC_NUM; ++i) {
		clients[i]._s_state = ST_INGAME;
		clients[i].x = (i - 10) * 1.5;
		clients[i].y = 0;
		clients[i].z = 0;
		clients[i].degree = 0;
		clients[i].chn = true;
		clients[i]._name[0] = 0;
		clients[i]._prev_remain = 0;
		add_timer(i, 100, EV_MOVE, i);
	}
}

int main()
{
	initialize_npc();
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	int client_id = 0;

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 9999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	OVER_EXP a_over;
	a_over._comp_type = OP_ACCEPT;
	a_over._wsabuf.buf = reinterpret_cast<CHAR*>(c_socket);
	AcceptEx(g_s_socket, c_socket, a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &a_over._over);

	vector <thread> worker_threads;
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(do_worker);

	thread timer_thread{ do_timer };
	timer_thread.join();

	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}