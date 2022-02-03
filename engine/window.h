#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <memory>
#include <SDL.h>
#include "renderer.h"

class Window
{
public:
	bool wantsToClose;
	
private:
	bool fullscreen;
	bool uncapped;
	SDL_Window* window;

	int frameRateChoice;
	double targetSpf;
	double realSpf;

public:
	std::unique_ptr<Renderer> renderer;
	
	Window(bool vsync = false);
	~Window();

	void ShowWindow(bool show = true);
	void SwapBuffers();
	void ChangeFramerate();
	bool HandleEvents(SDL_Event event);

	//Sleeps until it's time to process the next frame.
	void SleepUntilNextFrame();

	double GetSpf();
	
};

extern Window *mainWindow;

#endif // WINDOW_H_INCLUDED
