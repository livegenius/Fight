#ifndef HITBOX_RENDER_H_GUARD
#define HITBOX_RENDER_H_GUARD

#include "pipeset.hpp"
#include "vertex_buffer.h"
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vector>

class Renderer;
//Holds the state needed to render hitboxes for debug purposes.
class HitboxRenderer
{
public:
	HitboxRenderer(Renderer &renderer);

	enum 
	{
		gray = 0,
		green,
		red,
	};

	void GenerateHitboxVertices(const std::vector<float> &boxes, int pickedColor);
	void LoadHitboxVertices();
	void DontDraw();
	void Draw(const glm::mat4 &transform);
	void DrawAxisOnly(const glm::mat4 &transform, float alpha);

private:
	Renderer &renderer;
	
	PipeSet pipeset;
	vk::raii::Pipeline lines = nullptr;
	vk::raii::Pipeline filling = nullptr;

	AllocatedBuffer vertices;
	AllocatedBuffer indices;
	//int geoVaoId;

/* 	std::vector<uint16_t> clientElements;
	std::vector<float> clientQuads; */

	int quadsToDraw = 0; //Number of boxes to draw.
	size_t acumFloats = 0; //Numbers of floats written to the vertex buffer.

	struct PushConstants{
		glm::mat4 transform;
		float alpha;
	};
};

#endif /* HITBOX_RENDER_H_GUARD */
