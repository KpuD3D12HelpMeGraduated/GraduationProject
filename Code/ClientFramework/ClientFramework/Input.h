#pragma once
#include "Util.h"
#include "SFML.h"

class Input
{
public:
	vector<int> _states;

	//���� ������ �ʱ�ȭ
	void Init();

	//Ű �Է�
	void InputKey(shared_ptr<Timer> timerPtr, Obj* playerArr, shared_ptr<SFML> networkPtr);
};