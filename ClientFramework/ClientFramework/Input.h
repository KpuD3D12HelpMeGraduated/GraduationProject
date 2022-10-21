#pragma once
#include "Util.h"
#include "SFML.h"

class Input
{
public:
	vector<int> _states;

	//벡터 사이즈 초기화
	void Init();

	//키 입력
	void InputKey(shared_ptr<Timer> timerPtr, Obj* playerArr, shared_ptr<SFML> networkPtr);
};