#include "gl_renderer.h"
#include <glad/glad.h>

OpenglRenderer::OpenglRenderer()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}

OpenglRenderer::~OpenglRenderer()
{
	if(glcontext)
		SDL_GL_DeleteContext(glcontext);
}

bool OpenglRenderer::Init(SDL_Window *window_, int syncMode)
{
	window = window_;
	if(syncMode)
		SDL_GL_SetSwapInterval(1);

	glcontext = SDL_GL_CreateContext(window);
	if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
	{
		SDL_DestroyWindow( window );
		SDL_Quit();
		return false;
	}

	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	UpdateViewport(width, height);
	return true;
}

void OpenglRenderer::UpdateViewport(float width, float height)
{
	constexpr float asRatio = (float)internalWidth/(float)internalHeight;
	if(width/height > asRatio) //wider
	{
		glViewport((width-height*asRatio)/2, 0, height*asRatio, height);
	}
	else
	{
		glViewport(0, (height-width/asRatio)/2, width, width/asRatio);
	}
}

void OpenglRenderer::Submit()
{
	SDL_GL_SwapWindow(window);
}

bool OpenglRenderer::HandleEvents(SDL_Event event)
{
	switch(event.type)
	{
		case SDL_WINDOWEVENT:
			switch(event.window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					UpdateViewport(event.window.data1, event.window.data2);
					return true;
			}
			break;
	}
	return false;
}