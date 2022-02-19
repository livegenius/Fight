#include "gfx_handler.h"
#include <iostream>
#include <sol/sol.hpp>
#include <fstream>
#include <glm/mat4x4.hpp>

constexpr unsigned int vertexStride = 4 * sizeof(short);

GfxHandler::GfxHandler(Renderer *renderer_):
renderer(*renderer_),
vertices(renderer_)
{}

GfxHandler::~GfxHandler()
{
	renderer.Wait();
}

void GfxHandler::LoadLuaDefinitions(sol::state &lua)
{
	lua["lzs3"] = 1;
}

int GfxHandler::LoadGfxFromDef(std::filesystem::path defFile)
{
	auto folder = defFile.parent_path();
	sol::state lua;
	LoadLuaDefinitions(lua);
	auto result = lua.script_file(defFile.string());
	if(!result.valid()){
		sol::error err = result;
		std::cerr << "When loading " << defFile <<"\n";
		std::cerr << err.what() << std::endl;
		throw std::runtime_error("Lua syntax error.");
	}
	return LoadGfxFromLua(lua, folder);
}

int GfxHandler::LoadGfxFromLua(sol::state &lua, std::filesystem::path workingDir)
{
	sol::table graphics = lua["graphics"];

	int mapId = idMapList.size();
	idMapList.push_back({});

	std::vector<Renderer::LoadTextureInfo> infos;
	const auto textureIndex = textures.size();
	for(const auto &entry : graphics)
	{
		sol::table arr = entry.second;
		std::string imageFile = arr["image"];
		std::string vertexFile = arr["vertex"];
		int type = arr["type"].get_or(0);
		bool filter = arr["filter"].get_or(false);

		infos.push_back({});
		Renderer::LoadTextureInfo &info = infos.back();
		info.path = workingDir/imageFile;
		info.repeat = true;
		if(filter)
			info.linearFilter = true;
		
		if(type == 1)
			info.type = Renderer::TextureType::lzs3;
		else
			info.type = Renderer::TextureType::png;

		LoadToVertexBuffer(workingDir/vertexFile, mapId, textureIndex+infos.size()-1);
	}

	renderer.LoadTextures(infos, textures);
	return mapId;
}

void GfxHandler::LoadToVertexBuffer(std::filesystem::path file, int mapId, int textureIndex)
{
	int nSprites, nChunks;
	std::ifstream vertexFile(file, std::ios_base::binary);

	vertexFile.read((char *)&nSprites, sizeof(int));
	auto chunksPerSprite = new uint16_t[nSprites];
	vertexFile.read((char *)chunksPerSprite, sizeof(uint16_t)*nSprites);

	vertexFile.read((char *)&nChunks, sizeof(int));

	int vdStart = tempVDContainer.size();
	//tempVDContainer.push_back(std::make_unique<VertexData4[]>(nChunks*6));
	tempVDContainer.emplace_back(new VertexData4[nChunks*6]);
	int size = sizeof(VertexData4);

	vertexFile.read((char *)(tempVDContainer[vdStart].get()), nChunks*6*size);
	int chunkCount = 0;
	for(int i = 0; i < nSprites; i++)
	{
		uint16_t virtualId;
		vertexFile.read((char*)&virtualId, sizeof(virtualId));

		int trueId = vertices.Prepare(size*6*((int)chunksPerSprite[i]), vertexStride, tempVDContainer[vdStart].get()+6*chunkCount);
		chunkCount += chunksPerSprite[i];

		idMapList[mapId].insert({virtualId, {trueId, textureIndex}});
	}
	
	delete[] chunksPerSprite;
}

void GfxHandler::LoadingDone()
{
	//Count the number of the type of texture so it can go into the specialized texture array in the shader.
	int32_t iTextureNumber = 0;
	int32_t textureNumber = 0;
	for(auto &tex: textures)
		if(tex.format == vk::Format::eR8Uint)
			iTextureNumber += 1;
		else
			textureNumber += 1;
	
	sampler = renderer.CreateSampler();
	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetSpecializationConstants({iTextureNumber, textureNumber})
		.SetShaders("data/spirv/shader.vert.bin", "data/spirv/shader.frag.bin")
		.SetInputLayout(true, {vk::Format::eR16G16Sscaled, vk::Format::eR16G16Uscaled})
		.SetPushConstants({{.stageFlags = vk::ShaderStageFlagBits::eVertex, .size = sizeof(glm::mat4)}})
	;
	pBuilder.Build(pipeline, pipelineLayout, sets, setLayouts);
	pBuilder.UpdateSets({
		{vk::DescriptorImageInfo{
			.sampler = *sampler,
			.imageView = *textures[0].view,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		}, 0, 0},
	});

	vertices.Load();
	tempVDContainer.clear();
	loaded = true;
/*	glGenBuffers(1, &particleBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
	glBufferData(GL_ARRAY_BUFFER, particleAttributeSize, nullptr, GL_STREAM_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*4));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(2,1);
	glVertexAttribDivisor(3,1); */
}

void GfxHandler::Begin()
{
	cmd = &renderer.GetCommand();
	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
	cmd->bindVertexBuffers(0, vertices.buffer.buffer, {0});
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, sets, nullptr);
}

void GfxHandler::End()
{
	boundTexture = -1;
	boundProgram = -1;
	paletteSlot = -1;
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GfxHandler::Draw(int id, int defId, int _paletteSlot)
{
	
 	auto search = idMapList[defId].find(id);
	if (search != idMapList[defId].end())
	{
		auto meta = search->second;
		/* if(boundTexture != meta.textureIndex)
		{
			boundTexture = meta.textureIndex;
			//glBindTexture(GL_TEXTURE_2D, textures[boundTexture].id);
		}
		int nextProgram = 0;//textures[boundTexture].is8bpp;
		if(boundProgram != nextProgram)
		{
			boundProgram = nextProgram;
			if(nextProgram == 1)
			{
				indexedS.Use();
			}
			else
			{
				rectS.Use();
			}
		}
		if(nextProgram == 1 && paletteSlot != _paletteSlot)
		{
			paletteSlot = _paletteSlot;
			//glUniform1i(paletteSlotL, paletteSlot);
		} */

		auto what = vertices.Index(meta.trueId);
		cmd->draw(what.second, 1, what.first, 0);
	} 
}

void GfxHandler::DrawParticles(std::vector<Particle> &data, int id, int defId)
{
	auto size = data.size();
	if(size > 0){
		auto search = idMapList[defId].find(id);
		if (search != idMapList[defId].end())
		{
			if(size > maxParticles)
				size = maxParticles;
			auto meta = search->second;
			if(boundTexture != meta.textureIndex)
			{
				boundTexture = meta.textureIndex;
				//glBindTexture(GL_TEXTURE_2D, textures[boundTexture].id);
			}

			if(boundProgram != particleS.program)
			{
				boundProgram = particleS.program;
				particleS.Use();
			}

			/* glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle)*size, data.data()); */
			//vertices.DrawInstances(meta.trueId, size);
		}
	}
}

void GfxHandler::SetMulColor(float r, float g, float b, float a)
{
	//rectS.Use();
	//glUniform4f(rectMulColorL, r,g,b,a);
	//indexedS.Use();
	//glUniform4f(indexedMulColorL, r,g,b,a);
	boundProgram = -1;
}

int GfxHandler::GetVirtualId(int id, int defId)
{
	for(auto search: idMapList[defId])
	{
		if(search.second.trueId == id)
			return search.first;
	}
	return -1;
}

void GfxHandler::SetMatrix(const glm::mat4 &matrix)
{
	vk::ArrayProxy<const uint8_t> push(sizeof(matrix), (uint8_t*)&matrix);
	cmd->pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, push);
}