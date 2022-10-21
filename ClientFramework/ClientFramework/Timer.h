#pragma once
#include "Util.h"

class Timer
{
public:
	float	_deltaTime = 0.f;
	
	void InitTimer();
	
	void TimerUpdate();

	void ShowFps(WindowInfo windowInfo);

private:
	unsigned _int64	_frequency = 0;
	unsigned _int64	_prevCount = 0;
	

	unsigned _int32	_frameCount = 0;
	float	_frameTime = 0.f;
	unsigned _int32	_fps = 0;
};