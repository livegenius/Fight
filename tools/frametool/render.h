#ifndef RENDER_H_GUARD
#define RENDER_H_GUARD

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <gfx_handler.h>

class Render
{
private:
	glm::mat4 projection;
	GfxHandler gfx;

	float colorRgba[4];
	
	void SetModelView(glm::mat4&& view);

public:
	int spriteId;
	int x, offsetX;
	int y, offsetY;
	float scale;
	float scaleX, scaleY;
	float rotX, rotY, rotZ;
	int highLightN = -1;
	
	
	Render();

	void Draw();
	void UpdateProj(float w, float h);

	void DontDraw();
	void SetImageColor(float *rgbaArr);

	void LoadGraphics(std::filesystem::path path);
	void LoadPalette(std::filesystem::path path);
};

#endif /* RENDER_H_GUARD */
