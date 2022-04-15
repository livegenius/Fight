#include "gfx_handler.h"
#include "image.h"
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
	
	for(const auto &entry : graphics)
	{
		sol::table arr = entry.second;

		sol::optional<std::string> imageFile = arr["image"];
		sol::optional<std::string> vertexFile = arr["vertex"];
		if(imageFile && vertexFile)
		{
			int type = arr["type"].get_or(0);
			Renderer::LoadTextureInfo info{
				.path = workingDir/imageFile.value(),
				.type = Renderer::SrcTextureType::standard
			};

			int ByPP = ImageData::PeekBytesPerPixel(info.path);
			if(ByPP == 1)
			{
				LoadToVertexBuffer(workingDir/vertexFile.value(), mapId, index8);
				index8++;
			}
			else if(ByPP == 4) //32bit textures have negative indices to differenciate.
			{
				LoadToVertexBuffer(workingDir/vertexFile.value(), mapId, index32-1);
				index32--;
			}
			else
			{
				std::cerr << info.path << " : Unhandled texture byte depth "<<ByPP<<". Skipping...";
				continue;
			}
			infos.push_back(std::move(info));
		}
	}

	int spriteEndIndexNeg = -infos.size();
	sol::optional<std::vector<std::string>> imageListOpt = graphics["images"];
	if(imageListOpt)
	{
		for(const std::string &imageFile : imageListOpt.value())
		{
			auto path = workingDir/imageFile;
			infos.push_back({
				.path = path,
				.type = Renderer::SrcTextureType::standard
			});
			int ByPP = ImageData::PeekBytesPerPixel(path);
			assert(ByPP == 4 && "Unsupported ByPP");
		}
	}

	spriteEndIndexNeg += infos.size();

	renderer.LoadTextures(infos, textureAtlas);

	//Keep chunked textures separate.
	textureQuads.insert(textureQuads.end(), std::make_move_iterator(textureAtlas.end() - spriteEndIndexNeg), std::make_move_iterator(textureAtlas.end()));
	textureAtlas.erase(textureAtlas.end() - spriteEndIndexNeg, textureAtlas.end());

	//If there's a palette add it.
	auto palettesOptFp = lua.get<sol::optional<std::string>>("palettes");
	if(palettesOptFp)
	{
		auto start = paletteData.size();
		std::filesystem::path filepath = workingDir/palettesOptFp.value();
		std::basic_ifstream<uint8_t> paletteFile(filepath, std::ios_base::binary);
		if(paletteFile.is_open())
		{
			uint32_t size = -1;
			paletteFile.read((uint8_t*)&size, sizeof(size));
			if(size > 64)
				throw std::runtime_error("Palette entries exceed 64");

			size = 4*256*size;
			paletteData.resize(start+size);
			paletteFile.read(&paletteData[start], sizeof(uint8_t)*size);
		}
		else
			throw std::runtime_error(std::string("Can't open palette file ")+filepath.string());
	}
	return mapId;
}

void GfxHandler::LoadToVertexBuffer(std::filesystem::path file, int mapId, int textureIndex)
{
	int nSprites, nChunks;
	std::ifstream vertexFile(file, std::ios_base::binary);
	if(!vertexFile.is_open())
		throw std::runtime_error(file.string()+" : path doesn't exist.\n");

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
	vertices.Load();
	tempVDContainer.clear();
	SetupSpritePipeline();
	SetupParticlePipeline();
	
	loaded = true;
}

void GfxHandler::SetupSpritePipeline()
{
	
	assert(!paletteData.empty());
	palette = renderer.LoadTextureSingle(Renderer::LoadTextureInfo{
		.type = Renderer::SrcTextureType::palette,
		.width = 256,
		.height = static_cast<uint16_t>(paletteData.size()/(256*4)),
		.data = paletteData.data()
	});
	paletteData.~vector();

	//Count the number of the type of texture so it can go into the specialized texture array in the shader.
	int iTextureNumber = 0;
	std::stable_sort(textureAtlas.begin(), textureAtlas.end(), [](const Renderer::Texture& tex1, const Renderer::Texture& tex2){
		return (tex1.format > vk::Format::eR8Srgb) < (tex2.format > vk::Format::eR8Srgb);
	});
	for(auto &tex: textureAtlas)
		if(tex.format == vk::Format::eR8Uint)
			iTextureNumber += 1;
	int textureNumber = textureAtlas.size() - iTextureNumber;
	assert(iTextureNumber > 0);
	
	samplerI = renderer.CreateSampler();
	sampler = renderer.CreateSampler(vk::Filter::eLinear);
	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetSpecializationConstants({iTextureNumber, textureNumber})
		.SetShaders("data/spirv/sprite.vert.bin", "data/spirv/sprite.frag.bin")
		.SetInputLayout(true, {vk::Format::eR16G16Sint, vk::Format::eR16G16Sint})
		.SetPushConstants({
			{.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, .size = sizeof(pushConstants)},
		})
	;
	
	spritePipe.accessor = pBuilder.Build(spritePipe.pipeline, spritePipe.pipelineLayout, spritePipe.sets, spritePipe.setLayouts);

	std::vector<PipelineBuilder::WriteSetInfo> updateSetParams;
	updateSetParams.reserve(textureAtlas.size()+1);
	updateSetParams.push_back({vk::DescriptorImageInfo{ //Palette
		.sampler = *sampler,
		.imageView = *palette.view,
		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
	}, 0, 0});
	for(int i = 0; i < iTextureNumber; ++i) //Indexed
	{
		updateSetParams.push_back({vk::DescriptorImageInfo{
			.sampler = *samplerI,
			.imageView = *textureAtlas[i].view,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		}, 0, 1});
	}
	for(int i = iTextureNumber; i < textureAtlas.size(); ++i) //Truecolor
	{
		updateSetParams.push_back({vk::DescriptorImageInfo{
			.sampler = *sampler,
			.imageView = *textureAtlas[i].view,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		}, 0, 2});
	}
	
	pBuilder.UpdateSets(updateSetParams);
}

void GfxHandler::SetupParticlePipeline()
{
	maxParticles = renderer.uniformBufferSizeLimit/sizeof(Particle);

	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetSpecializationConstants({(int)textureQuads.size(), (int)maxParticles})
		.SetShaders("data/spirv/particle.vert.bin", "data/spirv/particle.frag.bin")
		.SetPushConstants({
			{.stageFlags = vk::ShaderStageFlagBits::eVertex, .size = sizeof(glm::mat4)},
		})
	;
	pBuilder.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	pBuilder.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOne;
	particlePipe.accessor = pBuilder.Build(particlePipe.pipeline, particlePipe.pipelineLayout, particlePipe.sets, particlePipe.setLayouts, {2});

	size_t bufSize = maxParticles * sizeof(Particle);
	particleProperties.Allocate(&renderer, bufSize,
	vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, renderer.bufferedFrames);

	std::vector<PipelineBuilder::WriteSetInfo> updateSetParams;
	updateSetParams.reserve(textureQuads.size()+2);
	for(short i = 0; i < renderer.bufferedFrames; i++)
	{
		updateSetParams.push_back({vk::DescriptorBufferInfo{ //Palette
			.buffer = particleProperties.buffer,
			.offset = renderer.PadToAlignment(bufSize)*i,
			.range = bufSize,
		}, 0, 0, i});
	}
	//for(short j = 0; j < 2; ++j)
	for(int i = 0; i < textureQuads.size(); ++i) //Indexed
	{
		updateSetParams.push_back({vk::DescriptorImageInfo{
			.sampler = *sampler,
			.imageView = *textureQuads[i].view,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		}, 1, 0});
	}
	pBuilder.UpdateSets(updateSetParams);
}

void GfxHandler::Begin()
{
	cmd = renderer.GetCommand();
	if(!cmd)
		return;

	void *propMem = particleProperties.Map();

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *spritePipe.pipeline);
	cmd->bindVertexBuffers(0, vertices.buffer.buffer, {0});
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *spritePipe.pipelineLayout, 0, {
		spritePipe.sets[spritePipe.accessor(0,0)]
	}, nullptr);
}

void GfxHandler::End()
{
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GfxHandler::Draw(int id, int defId)
{
	if(!cmd)
		return;
	auto search = idMapList[defId].find(id);
	if (search != idMapList[defId].end())
	{
		auto meta = search->second;
		if(meta.textureIndex >= 0)
		{
			pushConstants.shaderType = 0;
			pushConstants.textureIndex = meta.textureIndex;
		}
		else
		{
			pushConstants.shaderType = 1;
			pushConstants.textureIndex = -meta.textureIndex-1;
		}
		
		vk::ArrayProxy<const uint8_t> push(sizeof(pushConstants), (uint8_t*)&pushConstants);
		cmd->pushConstants(*spritePipe.pipelineLayout, vk::ShaderStageFlagBits::eFragment |  vk::ShaderStageFlagBits::eVertex, 0, push);

		auto what = vertices.Index(meta.trueId);
		cmd->draw(what.second, 1, what.first, 0);
	} 
}

void GfxHandler::DrawParticles(const std::vector<Particle> &data, int id, int defId)
{
	if(!cmd)
		return;
	auto frame = renderer.CurrentFrame();
	auto buf = particleProperties.Map(frame);
	auto srcSize = data.size()*sizeof(Particle);

	if(srcSize > particleProperties.copySize)
		srcSize = particleProperties.copySize;
	memcpy(buf, data.data(), srcSize);
	

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *particlePipe.pipeline);
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *particlePipe.pipelineLayout, 0, 
		{particlePipe.sets[particlePipe.accessor(0, frame)],
		particlePipe.sets[particlePipe.accessor(1,0)] 
	}, nullptr);

	vk::ArrayProxy<const uint8_t> push(sizeof(glm::mat4), (uint8_t*)&pushConstants.transform);
	cmd->pushConstants(*particlePipe.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, push);
	cmd->draw(data.size()*6, 1, 0, 0);

	return;

	auto size = data.size();
	if(size > 0){
		auto search = idMapList[defId].find(id);
		if (search != idMapList[defId].end())
		{
			/* if(size > maxParticles)
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

			glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle)*size, data.data()); */
			//vertices.DrawInstances(meta.trueId, size);
		}
	}
}

void GfxHandler::SetMulColor(float r, float g, float b, float a)
{
	pushConstants.mulColor = {r,g,b,a};
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
	pushConstants.transform = matrix;
}

void GfxHandler::SetPaletteSlot(int slot)
{
	pushConstants.paletteSlot = slot;
}

void GfxHandler::SetPaletteIndex(int index)
{
	pushConstants.paletteIndex = index;
}