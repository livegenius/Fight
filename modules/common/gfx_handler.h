#ifndef GFX_HANDLER_H_GUARD
#define GFX_HANDLER_H_GUARD

#include "particle.h"
#include "vao.h"
#include "texture.h"
#include <shader.h>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>

class GfxHandler{
private:
	struct VertexData4
	{
		unsigned short x,y,s,t;
	};

	struct spriteIdMeta
	{
		int trueId;
		int textureIndex;
	};

	std::vector<std::unique_ptr<VertexData4[]>> tempVDContainer;
	Vao vertices;

	//One for each def load.
	//Maps virtual id to real id;
	std::vector<std::unordered_map<int, spriteIdMeta>> idMapList;
	
	std::vector<Texture> textures;
	
	
	int boundTexture = -1;
	int boundProgram = -1;
	int paletteSlot = 0;
	int paletteSlotL;
	
	bool loaded = false;

	void LoadToVao(std::filesystem::path file, int mapId, int textureIndex);

	static constexpr int stride = sizeof(float)*4;
	static constexpr int maxParticles = 1024;
	static constexpr GLsizeiptr particleAttributeSize = stride*maxParticles; 
	static_assert(sizeof(Particle) == stride);

	GLuint particleBuffer;

public:

	Shader indexedS, rectS, particleS;

	GfxHandler();
	~GfxHandler();

	//Returns the id that identifies the def file
	int LoadGfxFromDef(std::filesystem::path defFile);
	void LoadingDone();

	void SetPaletteSlot(int palette);
	void Draw(int id, int defId = 0);
	void DrawParticles(std::vector<Particle> &data, int id, int defId = 0);
	void Begin();
	void End();

	bool isLoaded(){return loaded;}

	int GetVirtualId(int id, int defId = 0); //Avoid using this
};

#endif /* GFX_HANDLER_H_GUARD */
