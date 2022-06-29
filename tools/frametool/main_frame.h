#ifndef MAINFRAME_H_GUARD
#define MAINFRAME_H_GUARD

#include "render.h"
#include "draw_window.h"
#include "framedata.h"
#include "main_pane.h"

#include <glm/mat4x4.hpp>
#include <string>


class Renderer;
class MainFrame
{
public:
	MainFrame(Renderer *renderer);
	~MainFrame();
	
	void DrawBack();
	void DrawUi();

	void UpdateBackProj(float x, float y);
	void HandleMouseDrag(int x, int y, bool dragLeft, bool dragRight);
	bool HandleKeys(uint64_t vkey);

	void RightClick(int x, int y);
	void LoadSettings();

	void SetClientRect(int x, int y);

private:
	Framedata fd;
	struct vec2d
	{
		int x, y;
	}clientRect;

	float clearColor[4] = {1,1,1,1};
	int style_idx = 0;
	int zoom_idx = 3;

	Renderer *backendRenderer;
	Render render;
	FrameState currState;

	std::string currentFilePath;


	void Menu(unsigned int errorId);

	void RenderUpdate();
	void AdvancePattern(int dir);
	void AdvanceFrame(int dir);

	void SetZoom(int level);
	void LoadTheme(int i );
	void WarmStyle();
	void ChangeClearColor(float r, float g, float b);

	int mDeltaX = 0, mDeltaY = 0;
	int x=0, y=0;

	MainPane mainPane;
	
};


#endif /* MAINFRAME_H_GUARD */
