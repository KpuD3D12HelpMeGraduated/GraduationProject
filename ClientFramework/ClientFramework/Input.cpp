#include "Timer.h"
#include "Input.h"

void Input::Init()
{
	_states.resize(255);
}

void Input::InputKey(shared_ptr<Timer> timerPtr, Obj* playerArr, shared_ptr<SFML> networkPtr)
{
	HWND hwnd = GetActiveWindow();

	for (UINT key = 0; key < 255; key++)
	{
		if (GetAsyncKeyState(key) & 0x8000)
		{
			if (_states[key] == 1 || _states[key] == 2)
				_states[key] = 1;
			else
				_states[key] = 2;
		}
		else
		{
			int& state = _states[key];

			if (_states[key] == 1 || _states[key] == 2)
				_states[key] = 3;
			else
				_states[key] = 0;
		}
	}

	if (_states['W'] == 1)
	{
		playerArr[networkPtr->myClientId].transform.x += 5.0f * timerPtr->_deltaTime * cosf(XM_PI / 2.0f);
		playerArr[networkPtr->myClientId].transform.z += 5.0f * timerPtr->_deltaTime * sinf(XM_PI / 2.0f);

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		//p.degree = playerArr[myClientId].rotate.y;
		p.x = playerArr[networkPtr->myClientId].transform.x;
		p.y = playerArr[networkPtr->myClientId].transform.y;
		p.z = playerArr[networkPtr->myClientId].transform.z;
		networkPtr->send_packet(&p);
	}
	else if (_states['S'] == 1)
	{
		playerArr[networkPtr->myClientId].transform.x -= 5.0f * timerPtr->_deltaTime * cosf(XM_PI / 2.0f);
		playerArr[networkPtr->myClientId].transform.z -= 5.0f * timerPtr->_deltaTime * sinf(XM_PI / 2.0f);

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		//p.degree = playerArr[myClientId].rotate.y;
		p.x = playerArr[networkPtr->myClientId].transform.x;
		p.y = playerArr[networkPtr->myClientId].transform.y;
		p.z = playerArr[networkPtr->myClientId].transform.z;
		networkPtr->send_packet(&p);
	}
	else if (_states['A'] == 1)
	{
		playerArr[networkPtr->myClientId].transform.x -= 5.0f * timerPtr->_deltaTime * cosf(0.0f);
		playerArr[networkPtr->myClientId].transform.z -= 5.0f * timerPtr->_deltaTime * sinf(0.0f);

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		//p.degree = playerArr[myClientId].rotate.y;
		p.x = playerArr[networkPtr->myClientId].transform.x;
		p.y = playerArr[networkPtr->myClientId].transform.y;
		p.z = playerArr[networkPtr->myClientId].transform.z;
		networkPtr->send_packet(&p);
	}
	else if (_states['D'] == 1)
	{
		playerArr[networkPtr->myClientId].transform.x += 5.0f * timerPtr->_deltaTime * cosf(0.0f);
		playerArr[networkPtr->myClientId].transform.z += 5.0f * timerPtr->_deltaTime * sinf(0.0f);

		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		//p.degree = playerArr[myClientId].rotate.y;
		p.x = playerArr[networkPtr->myClientId].transform.x;
		p.y = playerArr[networkPtr->myClientId].transform.y;
		p.z = playerArr[networkPtr->myClientId].transform.z;
		networkPtr->send_packet(&p);
	}
}