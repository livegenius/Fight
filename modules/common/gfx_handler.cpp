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
			{.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, .size = sizeof(pcSprites)},
		})
	;
	spritePipe.pipeline = pBuilder.Build(spritePipe.pipeset);
/* 	pBuilder.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	pBuilder.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOne;
	pBuilder.BuildDerivate(spriteAdditive); */

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

bool GfxHandler::Begin()
{
	cmd = renderer.GetCommand();
	if(!cmd)
		return false;

	void *propMem = particleProperties.Map();

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *spritePipe.pipeline);
	cmd->bindVertexBuffers(0, vertices.buffer.buffer, {0});
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *spritePipe.pipeset.pipelineLayout, 0, {
		spritePipe.pipeset.GetSet(0,0)
	}, nullptr);
	//boundPipe = normal;
	return true;
}

void GfxHandler::Draw(int id, int defId)
{
	auto search = idMapList[defId].find(id);
	if (search != idMapList[defId].end())
	{
		auto meta = search->second;
		if(meta.textureIndex >= 0)
		{
			pcSprites.shaderType = 0;
			pcSprites.textureIndex = meta.textureIndex;
		}
		else
		{
			pcSprites.shaderType = 1;
			pcSprites.textureIndex = -meta.textureIndex-1;
		}

		pcSprites.mulColor = mulColor;
		pcSprites.mulColor.a *= blendingModeFactor;
		
		cmd->pushConstants(*spritePipe.pipeset.pipelineLayout, vk::ShaderStageFlagBits::eFragment |  vk::ShaderStageFlagBits::eVertex,
			0, sizeof(pcSprites), &pcSprites);

		auto what = vertices.Index(meta.trueId);
		cmd->draw(what.second, 1, what.first, 0);
	} 
}

void GfxHandler::SetupParticlePipeline()
{
	maxParticles = renderer.uniformBufferSizeLimit/sizeof(ParticleGroup::Particle);

	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetSpecializationConstants({(int)textureQuads.size(), (int)maxParticles})
		.HintDescriptorType(0, 0, vk::DescriptorType::eUniformBufferDynamic)
		.SetShaders("data/spirv/particle.vert.bin", "data/spirv/particle.frag.bin")
		.SetPushConstants({
			{.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, .size = sizeof(pcParticles)},
		})
	;
	particlePipe.pipeline = pBuilder.Build(particlePipe.pipeset, {2});

	size_t bufSize = 16 * maxParticles * sizeof(ParticleGroup::Particle);
	particleProperties.Allocate(&renderer, bufSize,
	vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, renderer.bufferedFrames);

	std::vector<PipelineBuilder::WriteSetInfo> updateSetParams;
	updateSetParams.reserve(textureQuads.size()+2);
	for(short i = 0; i < renderer.bufferedFrames; i++) //Particle pos uniform buffer
	{
		updateSetParams.push_back({vk::DescriptorBufferInfo{ 
			.buffer = particleProperties.buffer,
			.offset = renderer.PadToAlignment(bufSize)*i,
			.range = renderer.uniformBufferSizeLimit,
		}, 0, 0, i});
	}

	for(int i = 0; i < textureQuads.size(); ++i) //Textures
	{
		updateSetParams.push_back({vk::DescriptorImageInfo{
			.sampler = *sampler,
			.imageView = *textureQuads[i].view,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		}, 1, 0});
	}
	pBuilder.UpdateSets(updateSetParams);
}

void GfxHandler::DrawParticles(const ParticleGroup &data)
{
	auto frame = renderer.CurrentFrame();
	auto buf = (uint8_t*)particleProperties.Map(frame);

	auto drawList = data.FillBuffer(buf, renderer.uniformBufferSizeLimit, particleProperties.copySize, renderer.uniformBufferAlignment);
	particleProperties.Unmap();
	if(drawList.empty())
		return;

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *particlePipe.pipeline);
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *particlePipe.pipeset.pipelineLayout, 1, 
		particlePipe.pipeset.GetSet(1,0), nullptr);

	pcParticles.transform = pcSprites.transform,
	pcParticles.textureId = -1;
	cmd->pushConstants(*particlePipe.pipeset.pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		offsetof(decltype(pcParticles), transform), sizeof(pcParticles.transform), &pcParticles.transform);

	for(const ParticleGroup::DrawInfo &drawInfo : drawList)
	{
		//Do not exceed addressable range;
		if(renderer.uniformBufferSizeLimit + drawInfo.bufferOffset > particleProperties.copySize)
			break;

		if(pcParticles.textureId != drawInfo.particleType)
		{
			pcParticles.textureId = drawInfo.particleType;
			cmd->pushConstants(*particlePipe.pipeset.pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 
				offsetof(decltype(pcParticles), textureId), sizeof(pcParticles.textureId), &pcParticles.textureId);
		}
		cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *particlePipe.pipeset.pipelineLayout,
			0, particlePipe.pipeset.GetSet(0,frame), drawInfo.bufferOffset);
		cmd->draw(drawInfo.particleAmount*6, 1, 0, 0);
	}
}

void GfxHandler::SetMulColor(float r, float g, float b, float a)
{
	mulColor = {r*a,g*a,b*a,a};
}

void GfxHandler::SetMulColorRaw(float r, float g, float b, float a)
{
	mulColor = {r,g,b,a};
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
	pcSprites.transform = matrix;
}

void GfxHandler::SetPaletteSlot(int slot)
{
	pcSprites.paletteSlot = slot;
}

void GfxHandler::SetPaletteIndex(int index)
{
	pcSprites.paletteIndex = index;
}

void GfxHandler::SetBlendingMode(int mode)
{
	switch(mode)
	{
		case normal:			
			blendingModeFactor = 1.f;
			break;
		case additive:
			blendingModeFactor = 0.f;
			break;
	};
	/* if(boundPipe != mode)
	{
		boundPipe = mode;
		switch(mode)
		{
			case normal:			
				cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *spritePipe.pipeline);
				break;
			case additive:
				cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *spriteAdditive);
				break;
		};
	} */
}