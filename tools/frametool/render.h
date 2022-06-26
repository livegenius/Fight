#ifndef RENDER_H_GUARD
#define RENDER_H_GUARD

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <memory>

class GfxHandler;
class Renderer;
class HitboxRenderer;

class Render
{
private:
	glm::mat4 projection;
	std::unique_ptr<GfxHandler> gfx;
	std::unique_ptr<HitboxRenderer> hr;

	float colorRgba[4];
	
public:
	int spriteId;
	int x, offsetX;
	int y, offsetY;
	float scale;
	float scaleX, scaleY;
	float rotX, rotY, rotZ;
	int highLightN = -1;
	
	
	Render(Renderer*);
	~Render();

	void Draw();
	void UpdateProj(float w, float h);

	void DontDraw();
	void SetImageColor(float *rgbaArr);

	void LoadGraphics(std::filesystem::path path);

	enum // Dupe code, but whatever
	{
		gray = 0,
		green,
		red,
	};
	void GenerateHitboxVertices(const std::vector<int> &boxes, int color);
	void LoadHitboxVertices();
};

#endif /* RENDER_H_GUARD */
