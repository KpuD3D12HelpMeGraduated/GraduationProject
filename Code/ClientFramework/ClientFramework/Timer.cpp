#include "Timer.h"

void Timer::InitTimer()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));
}

void Timer::TimerUpdate()
{
	unsigned _int64 currentCount;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	_prevCount = currentCount;

	_frameCount++;
	_frameTime += _deltaTime;

	if (_frameTime >= 1.f)
	{
		_fps = static_cast<unsigned _int32>(_frameCount / _frameTime);

		_frameTime = 0.f;
		_frameCount = 0;
	}
}

void Timer::ShowFps(WindowInfo windowInfo)
{
	UINT fps = _fps;

	WCHAR text[100] = L"";
	wsprintf(text, L"FPS : %d", fps);

	SetWindowText(windowInfo.hwnd, text);
}