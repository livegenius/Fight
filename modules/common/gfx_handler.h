#ifndef GFX_HANDLER_H_GUARD
#define GFX_HANDLER_H_GUARD

#include "vk/renderer.h"

#include "particle.h"
#include "vertex_buffer.h"

#include <shader.h>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <sol/sol.hpp>

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

	std::vector<std::unique_ptr<VertexData4[]>> tempVDContainer;
	VertexBuffer vertices;

	//One for each def load.
	//Maps virtual id to real id;
	std::vector<std::unordered_map<int, spriteIdMeta>> idMapList;
	std::vector<int> textureHandles;
	
	int boundTexture = -1;
	int boundProgram = -1;
	int paletteSlot = -1;
	
	bool loaded = false;

	void LoadToVertexBuffer(std::filesystem::path file, int mapId, int textureIndex);

	static constexpr int stride = sizeof(float)*6;
	static constexpr int maxParticles = 512;
	static constexpr int particleAttributeSize = stride*maxParticles; 
	static_assert(sizeof(Particle) == stride);

	unsigned int particleBuffer;

public:
	int indexedP;
	Shader indexedS, rectS, particleS;

	GfxHandler(Renderer*);
	~GfxHandler();

	//Returns the id that identifies the def file
	int LoadGfxFromDef(std::filesystem::path defFile);
	int LoadGfxFromLua(sol::state &lua, std::filesystem::path workingDir);
	static void LoadLuaDefinitions(sol::state &lua);
	void LoadingDone();

	void Draw(int id, int defId = 0, int paletteSlot = 0);
	void DrawParticles(std::vector<Particle> &data, int id, int defId = 0);
	void Begin();
	void End();

	bool isLoaded(){return loaded;}
	void SetMulColor(float r, float g, float b, float a = 1.f);

	int GetVirtualId(int id, int defId = 0); //Avoid using this
};

#endif /* GFX_HANDLER_H_GUARD */
