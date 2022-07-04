#ifndef MAINPANE_H_GUARD
#define MAINPANE_H_GUARD
#include "draw_window.h"
#include <framedata_io.h>
#include "render.h"
#include <string>
#include <vector>

//This is the main pane on the left
class MainPane : DrawWindow
{
public:
	MainPane(Render* render, Framedata *frameData, FrameState &fs);
	void Draw();

	void RegenerateNames();

private:

	void DrawFrame(Frame &frame);
	bool copyThisFrame = true;
	std::vector<std::string> decoratedNames;
	std::vector<unsigned int> decoratedNameInfos;
};

#endif /* MAINPANE_H_GUARD */
