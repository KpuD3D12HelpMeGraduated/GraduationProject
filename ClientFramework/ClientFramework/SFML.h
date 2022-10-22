#pragma once
#include "Util.h"
#include <iostream>

class SFML
{
public:
	sf::TcpSocket socket;
	int myClientId;

	void ConnectServer() //서버에 접속시 보내주는 부분
	{
		wcout.imbue(locale("korean"));
		sf::Socket::Status status = socket.connect("127.0.0.1", PORT_NUM);
		socket.setBlocking(false);

		if (status != sf::Socket::Done) {
			wcout << L"서버와 연결할 수 없습니다.\n";
			while (true);
		}

		CS_LOGIN_PACKET p;
		p.size = sizeof(CS_LOGIN_PACKET);
		p.type = CS_LOGIN;
		strcpy_s(p.name, "a");
		send_packet(&p);
	}

	void ReceiveServer(Obj* playerArr, Obj* npcArr) //서버에서 받는거, clientMain
	{
		char net_buf[BUF_SIZE];
		size_t	received;

		auto recv_result = socket.receive(net_buf, BUF_SIZE, received);
		if (recv_result == sf::Socket::Error)
		{
			wcout << L"Recv 에러!";
			while (true);
		}
		if (recv_result != sf::Socket::NotReady)
			if (received > 0) process_data(net_buf, received, playerArr, npcArr);
	}

	void process_data(char* net_buf, size_t io_byte, Obj* playerArr, Obj* npcArr)
	{
		char* ptr = net_buf;
		static size_t in_packet_size = 0;
		static size_t saved_packet_size = 0;
		static char packet_buffer[BUF_SIZE];

		while (0 != io_byte) {
			if (0 == in_packet_size) in_packet_size = ptr[0];
			if (io_byte + saved_packet_size >= in_packet_size) {
				memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
				ProcessPacket(packet_buffer, playerArr, npcArr);
				ptr += in_packet_size - saved_packet_size;
				io_byte -= in_packet_size - saved_packet_size;
				in_packet_size = 0;
				saved_packet_size = 0;
			}
			else {
				memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
				saved_packet_size += io_byte;
				io_byte = 0;
			}
		}
	}

	//서버에서 데이터 받을때(패킷종류별로 무슨 작업 할건지 ex: 이동 패킷, 로그인 패킷 how to 처리)
	void ProcessPacket(char* ptr, Obj* playerArr, Obj* npcArr)
	{
		static bool first_time = true;
		switch (ptr[1])
		{
		case SC_LOGIN_OK:
		{
			SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(ptr);
			myClientId = packet->id;
			playerArr[myClientId].on = true;
			playerArr[myClientId].transform.x = packet->x;
			playerArr[myClientId].transform.y = packet->y;
			playerArr[myClientId].transform.z = packet->z;

			break;
		}
		case SC_ADD_OBJECT:
		{
			SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
			int id = my_packet->id;
			printf_s("%d\n", id);
			if (id < PLAYERMAX) {
				playerArr[id].on = true;
				playerArr[id].transform.x = my_packet->x;
				playerArr[id].transform.y = my_packet->y;
				playerArr[id].transform.z = my_packet->z;
			}
			else if (id >= PLAYERMAX)
			{
				npcArr[id - PLAYERMAX].on = true;
				npcArr[id - PLAYERMAX].transform.x = my_packet->x;
				npcArr[id - PLAYERMAX].transform.y = my_packet->y;
				npcArr[id - PLAYERMAX].transform.z = my_packet->z;
			}

			break;
		}
		case SC_MOVE_OBJECT:
		{
			SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
			int id = my_packet->id;
			if (id < PLAYERMAX)
			{
				playerArr[id].transform.x = my_packet->x;
				playerArr[id].transform.y = my_packet->y;
				playerArr[id].transform.z = my_packet->z;
				//playerArr[id].rotate.y = my_packet->degree;
			}
			else if (id >= PLAYERMAX)
			{
				npcArr[id - PLAYERMAX].transform.x = my_packet->x;
				npcArr[id - PLAYERMAX].transform.y = my_packet->y;
				npcArr[id - PLAYERMAX].transform.z = my_packet->z;
			}
			
			break;
		}
		case SC_REMOVE_OBJECT:
		{
			SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
			int id = my_packet->id;
			playerArr[id].on = false;
			break;
		}
		default:
			printf("Unknown PACKET type [%d]\n", ptr[1]);
		}
	}

	//서버로 패킷 보내줄 때
	void send_packet(void* packet)
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(packet);
		size_t sent = 0;
		socket.send(packet, p[0], sent);
	}
};