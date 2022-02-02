#ifndef CONTEXT_H_GUARD
#define CONTEXT_H_GUARD

#include <SDL.h>

constexpr int internalWidth = 480;
constexpr int internalHeight = 270;

class Renderer
{
public:
	static Renderer* NewRenderer();

	virtual bool Init(SDL_Window *, int syncMode) = 0;
	virtual void Submit() = 0;
	virtual bool HandleEvents(SDL_Event) = 0;
};



#endif /* CONTEXT_H_GUARD */
