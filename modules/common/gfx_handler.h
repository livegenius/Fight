#ifndef GFX_HANDLER_H_GUARD
#define GFX_HANDLER_H_GUARD

#include "vk/renderer.h"

#include "particle.h"
#include "vertex_buffer.h"

#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <sol/sol.hpp>
#include <glm/mat4x4.hpp>

class GfxHandler{
private:
	struct VertexData4
	{
		short x,y;
		unsigned short s,t;
	};

	struct spriteIdMeta
	{
		int trueId;
		int textureIndex;
	};

	Renderer &renderer;
	const vk::CommandBuffer *cmd;
	
	struct Pipeline{
		vk::raii::Pipeline pipeline = nullptr;
		vk::raii::PipelineLayout pipelineLayout = nullptr;
		std::vector<vk::raii::DescriptorSetLayout> setLayouts;
		std::vector<vk::DescriptorSet> sets;
		std::function <int(int,int)> accessor;
	}spritePipe, particlePipe;


	std::vector<std::unique_ptr<VertexData4[]>> tempVDContainer;
	VertexBuffer vertices;
	
	AllocatedBuffer particleProperties;
	size_t maxParticles = 0;

	//One for each def load.
	//Maps virtual id to real id;
	std::vector<std::unordered_map<int, spriteIdMeta>> idMapList;

	vk::raii::Sampler samplerI = nullptr;
	vk::raii::Sampler sampler = nullptr;
	std::vector<Renderer::Texture> textureAtlas;
	std::vector<Renderer::Texture> textureQuads;
	size_t index8 = 0, index32 = 0; //Counters so sprites know what texture to access.

	Renderer::Texture palette;
	std::vector<uint8_t> paletteData;

	bool loaded = false;

	struct{
		glm::mat4 transform;
		int shaderType = 0;
		int textureIndex = 0;
		int paletteSlot = 0;
		int paletteIndex = 0;
		glm::vec4 mulColor ;
	} pushConstants;

	void LoadToVertexBuffer(std::filesystem::path file, int mapId, int textureIndex);

	void SetupSpritePipeline();
	void SetupParticlePipeline();

public:
	GfxHandler(Renderer*);
	~GfxHandler();

	//Returns the id that identifies the def file
	int LoadGfxFromDef(std::filesystem::path defFile);
	int LoadGfxFromLua(sol::state &lua, std::filesystem::path workingDir);
	static void LoadLuaDefinitions(sol::state &lua);
	void LoadingDone();

	void SetPaletteSlot(int slot);
	void SetPaletteIndex(int index);
	void SetMatrix(const glm::mat4 &matrix);
	void Draw(int id, int defId = 0);
	void DrawParticles(const std::vector<ParticleGroup::Particle> &data);
	void Begin();
	void End();

	bool isLoaded(){return loaded;}
	void SetMulColor(float r, float g, float b, float a = 1.f);

	int GetVirtualId(int id, int defId = 0); //Avoid using this
};

#endif /* GFX_HANDLER_H_GUARD */
