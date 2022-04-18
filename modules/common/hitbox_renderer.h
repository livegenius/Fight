#ifndef HITBOX_RENDER_H_GUARD
#define HITBOX_RENDER_H_GUARD
#include <vector>
#include "vertex_buffer.h"
#include "vk/renderer.h"
#include <glm/mat4x4.hpp>

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

private:
	Renderer &renderer;
	
	struct Pipeline{
		vk::raii::Pipeline pipeline = nullptr;
		vk::raii::PipelineLayout pipelineLayout = nullptr;
		std::vector<vk::raii::DescriptorSetLayout> setLayouts;
		std::vector<vk::DescriptorSet> sets;
		std::function <int(int,int)> accessor;
	}lines;
	vk::raii::Pipeline filling = nullptr;

	AllocatedBuffer vertices;
	AllocatedBuffer indices;
	//int geoVaoId;

/* 	std::vector<uint16_t> clientElements;
	std::vector<float> clientQuads; */

	int quadsToDraw = 0; //Number of boxes to draw.
	size_t acumFloats = 0; //Numbers of floats written to the vertex buffer.
	float zOrder = 0; 

	struct PushConstants{
		glm::mat4 transform;
		float alpha;
	};
};

#endif /* HITBOX_RENDER_H_GUARD */
