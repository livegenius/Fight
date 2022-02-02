#ifndef GL_CONTEXT_H_GUARD
#define GL_CONTEXT_H_GUARD

#include "../renderer.h"
#include <SDL.h>

class OpenglRenderer : public Renderer
{
	SDL_Window *window;
	SDL_GLContext glcontext = nullptr;
	void UpdateViewport(float width, float height);

public:
	OpenglRenderer();
	~OpenglRenderer();

	bool Init(SDL_Window *, int syncMode);
	void Submit();
	bool HandleEvents(SDL_Event);
};

#endif /* GL_CONTEXT_H_GUARD */
