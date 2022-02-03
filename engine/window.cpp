#include "window.h"
#include "raw_input.h"

#include <iostream>

//#define GENERIC_SLEEP
#if defined (_WIN32) && !defined(GENERIC_SLEEP)
	#include <windows.h>
	#include <timeapi.h>
#else
	#include <thread>
	#include <chrono>
#endif

Window *mainWindow = nullptr;

Window::Window(bool vsync) :
wantsToClose(false),
fullscreen(false),
uncapped(false),
window(nullptr),
frameRateChoice(0),
targetSpf(0.01666),
realSpf(0),
renderer(Renderer::NewRenderer())
{
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER))
	{
		std::cerr << SDL_GetError() <<"\n";
		throw std::runtime_error("Couldn't init SDL.");
	}

	uint32_t flags = SDL_WINDOW_RESIZABLE;
	#if defined (OPENGL_RENDERER)
		flags |= SDL_WINDOW_OPENGL;
	#elif defined (VULKAN_RENDERER)
		flags |= SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI;
	#endif

	if(fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow(
		"Fighting game",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, internalWidth*2, internalHeight*2, flags);

	if(!window)
	{
		std::cerr << SDL_GetError();
		SDL_Quit();
		throw std::runtime_error("Couldn't create window.");
	}

	renderer->Init(window, vsync);
}

Window::~Window()
{
	SDL_DestroyWindow( window );
	SDL_Quit();
}

void Window::ShowWindow(bool show)
{
	if(show)
		SDL_ShowWindow(window);
	else
		SDL_HideWindow(window);
}

#if defined (_WIN32) && !defined(GENERIC_SLEEP)
UINT GetMinTimer(){
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	return tc.wPeriodMin;
};
LONGLONG GetFreq(){
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}
auto frequency = GetFreq();
UINT minTimer = GetMinTimer(); 
LARGE_INTEGER startClockHr{};
void Window::SleepUntilNextFrame()
{
	timeBeginPeriod(minTimer);
	ULONGLONG targetCount = frequency*(targetSpf);
	ULONGLONG dif; 

	LARGE_INTEGER nowTicks;
	QueryPerformanceCounter(&nowTicks);
	if((dif = nowTicks.QuadPart - startClockHr.QuadPart) < targetCount)
	{
		LONG sleepTime = ((targetCount-dif)/(frequency/1000))-1;
		if(sleepTime>0)
			Sleep(sleepTime);
	}

	while((dif = nowTicks.QuadPart - startClockHr.QuadPart) < targetCount)
		QueryPerformanceCounter(&nowTicks); 

	realSpf = (double)dif/(double)frequency;
	QueryPerformanceCounter(&startClockHr);
	timeEndPeriod(minTimer);
}
#else
std::chrono::time_point<std::chrono::high_resolution_clock> startClock(std::chrono::high_resolution_clock::now()); 
void Window::SleepUntilNextFrame()
{
	constexpr double factor = 0.9; 
	if(!uncapped)
	{
		std::chrono::duration<double> targetDur(targetSpf);
		std::chrono::duration<double> dur; 
		auto now = std::chrono::high_resolution_clock::now();
		if((dur = now - startClock) < targetDur)
		{
			std::this_thread::sleep_for((targetDur-dur)*factor);
		}
		while( (dur = std::chrono::high_resolution_clock::now() - startClock) <= targetDur); 
	
		realSpf = dur.count();
	}
	else
	{
		realSpf = std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-startClock).count();
	}
	startClock = std::chrono::high_resolution_clock::now();
}
#endif

void Window::SwapBuffers()
{
	renderer->Submit();
}

double Window::GetSpf()
{
	return realSpf;
}

void Window::ChangeFramerate()
{
	constexpr double framerateList[4] = {
		0.01666, 
		1.0/30.0, 
		1.0/10.0,
		1.0/2.0
	};

	frameRateChoice++;
	if(frameRateChoice >= 4)
		frameRateChoice = 0;
	targetSpf = framerateList[frameRateChoice];
}


bool Window::HandleEvents(SDL_Event event)
{
	return renderer->HandleEvents(event);
}