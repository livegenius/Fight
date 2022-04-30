#ifndef HUD_H_INCLUDED
#define HUD_H_INCLUDED

#include <filesystem>
#include <vertex_buffer.h>
#include <renderer.h>
#include <glm/mat4x4.hpp>

class Hud
{
 	struct Coord{
		float x,y,s,t;
	};

	struct Bar
	{
		int pos;
		float w,h; //original size;
		float x,y;
		float tx,ty;
		bool flipped;
		bool inverted;
		int id;
	};

	vk::raii::Sampler sampler = nullptr;
	struct {
		vk::raii::Pipeline pipeline = nullptr;
		vk::raii::PipelineLayout pipelineLayout = nullptr;
		std::vector<vk::raii::DescriptorSetLayout> setLayouts;
		std::vector<vk::DescriptorSet> sets;
		std::function <int(int,int)> accessor;
	}pipe;

	struct{
		glm::mat4 transform;
	} pushConstants;
	
	float gScale;
	float width;
	float height;

	//int startId;
	size_t location;

	std::vector<Bar> barData;
	std::vector<Coord> coords;
	int staticCount = 0;
	VertexBuffer vao;

	Renderer &renderer;
	Renderer::Texture texture;

	void CreatePipeline();

public:
	Hud(Renderer *renderer);
	Hud(Renderer *renderer, std::filesystem::path file);
	void Load(std::filesystem::path file);
	void Draw();
	void ResizeBarId(int id, float horizPercentage); 
	void SetMatrix(const glm::mat4 &matrix);
};

#endif // HUD_H_INCLUDED
