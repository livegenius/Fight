#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <SDL.h>
#include <chrono>
#include "main_frame.h"

class Renderer;
class Window
{
private:
	Renderer *renderer;
	SDL_Window* window;
	std::unique_ptr<MainFrame> mf;

	int frameRateChoice;
	double targetSpf;
	double realSpf;
	std::chrono::time_point<std::chrono::high_resolution_clock> startClock; 
	
public:
	Window();
	~Window();

	void Render();
	void SwapBuffers();
	void ChangeFramerate();

	//Sleeps until it's time to process the next frame.
	void SleepUntilNextFrame();
	void UpdateClientRect();

	//Returns true until it gets a quit event.
	bool PollEvents();

	double GetSpf();
};

extern Window *mainWindow;

#endif // WINDOW_H_INCLUDED
