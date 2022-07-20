#include "hud.h"
#include "window.h"
#include <sol/sol.hpp>
#include <iostream>
#include <unordered_set>
#include <renderer.h>

constexpr int tX[] = {0,1,1, 1,0,0};
constexpr int tY[] = {0,0,1, 1,1,0};

Hud::Hud(Renderer *renderer):
renderer(*renderer),
vao(renderer)
{}
Hud::Hud(Renderer *renderer, std::filesystem::path file):
renderer(*renderer),
vao(renderer)
{
	Load(file);
}

void Hud::Load(std::filesystem::path file)
{
	sol::state lua;
	lua.open_libraries(sol::lib::base);
	auto result = lua.script_file(file.string());
	if(!result.valid()){
		sol::error err = result;
		std::cerr << "When loading " << file <<"\n";
		std::cerr << err.what() << std::endl;
		throw std::runtime_error("Lua syntax error.");
	}

	struct OriginalSize
	{
		float w,h,x,y,tx,ty;
		bool flipped;
		bool inverted;
	};
	std::vector<OriginalSize> sizes;

	coords.reserve(6*16);
	std::unordered_set<int> barSet;

	sol::table textureT = lua["texture"];
	width = textureT["width"];
	height = textureT["height"];
	gScale = textureT["scale"].get_or(1.f);
// 	float xScale = gScale;
//	float yScale = gScale;
	sol::table rects = textureT["rects"];
	int indexCounter = 0;
	for(const auto &entry : rects)
	{
		sol::table rect = entry.second;
		float x,y,w,h;

		sol::table rectCoords = rect["coords"];
		x = rectCoords["x"];
		y = height - rectCoords["y"].get<float>();
		w = rectCoords["w"];
		h = rectCoords["h"];

		bool fromBottom = rect["fromBottom"].get_or(false);
		bool fromRight = rect["fromRight"].get_or(false);
		bool mirror = rect["mirror"].get_or(true);
		//bool portrait = rect["portrait"].get_or(false);

		float posX = 0, posY = 0;
		sol::optional<sol::table> posOpt = rect["pos"];
		if(posOpt)
		{
			sol::table pos = posOpt->as<sol::table>();
			posX = pos["x"].get_or(0.f);
			posY = pos["y"].get_or(0.f);
			posX *= gScale;
			posY *= gScale;
		}

		if(fromBottom)
			posY += h*gScale;
		else
			posY += internalHeight;

		if(fromRight)
		{
			mirror = false;
			posX = internalWidth - posX - w*gScale;
		}
		for(int i = 0; i < 6; i++)
		{
			coords.push_back({
				posX + gScale*w*tX[i],
				posY - gScale*h*tY[i],
				(x + w*tX[i])/width,
				(y - h*tY[i])/height,
			});
		}
		sizes.push_back({w,h,posX,posY,x,y,false});
		if(rect["isBar"].get_or(false))
		{
			barSet.insert(coords.size()-6);
			sizes.back().inverted = rect["inverted"].get_or(false);
		}
			
		//else
		staticCount += 6;

		if(mirror)
		{
			for(int i = 0; i < 6; i++)
			{
				coords.push_back({
					internalWidth - (posX + gScale*w*tX[i]),
					posY - gScale*h*tY[i],
					(x + w*tX[i])/width,
					(y - h*tY[i])/height,
				});
			}
			sizes.push_back({w,h,posX,posY,x,y,true});
			if(rect["isBar"].get_or(false))
			{
				barSet.insert(coords.size()-6);
				sizes.back().inverted = rect["inverted"].get_or(false);
			}
			//else
			staticCount += 6;
		}
	}

	int startId = -1;
	for(int i = 0; i < coords.size(); i+=6)
	{
		if(barSet.count(i))
		{
			auto &original = sizes[i/6];
			barData.push_back({i, original.w, original.h,
				original.x, original.y, original.tx, original.ty, original.flipped, original.inverted});
			continue;
		}

		int id = vao.Prepare(sizeof(Coord)*6, sizeof(Coord), &coords[i]);
		if(startId == -1)
			startId = id;
	}
	
	for(auto &data : barData)
		data.id = vao.Prepare(sizeof(Coord)*6, sizeof(Coord), &coords[data.pos]);
	vao.LoadHostVisible(renderer.bufferedFrames);
	location = vao.Index(startId).first;

	Renderer::LoadTextureInfo opt{
		.path = file.parent_path()/textureT["file"].get<std::string>()
	};
	texture = std::move(renderer.LoadTextureSingle(opt));

	CreatePipeline();
}

void Hud::CreatePipeline()
{
	sampler = renderer.CreateSampler(vk::Filter::eLinear);
	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetShaders("data/spirv/hud.vert.bin", "data/spirv/hud.frag.bin")
		.SetInputLayout(true, {vk::Format::eR32G32Sfloat, vk::Format::eR32G32Sfloat})
		.SetPushConstants({
			{.stageFlags = vk::ShaderStageFlagBits::eVertex /* | vk::ShaderStageFlagBits::eFragment */, .size = sizeof(pushConstants)},
		})
	;
	pBuilder.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	pipeline = pBuilder.Build(pipeset);

	std::vector<PipelineBuilder::WriteSetInfo> updateSetParams;
	updateSetParams.push_back({vk::DescriptorImageInfo{
		.sampler = *sampler,
		.imageView = *texture.view,
		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
	}, 0, 0});

	pBuilder.UpdateSets(updateSetParams);
}

void Hud::Draw()
{
	auto cmd = renderer.GetCommand();
	if(!cmd)
		return;

	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
	cmd->bindVertexBuffers(0, vao.buffer.buffer, renderer.PadToAlignment(vao.buffer.copySize)*renderer.CurrentFrame());
	cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeset.layout, 0, {
		pipeset.Get(0, 0)
	}, nullptr);
	cmd->pushConstants(*pipeset.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstants), &pushConstants);
	cmd->draw(staticCount, 1, location, 0);	
}

void Hud::ResizeBarId(int id, float horizPercentage)
{
	auto &bar = barData[id]; 
	int start = bar.pos;
	float internalW = 0;
	float flipFactor = 1;
	if(bar.flipped)
	{
		internalW = internalWidth;
		flipFactor = -1;
	}
	
	if(!bar.inverted)
		for(int i = 0; i < 6; ++i)
		{
			coords[start+i].x = internalW + flipFactor*(bar.x + gScale*tX[i]*bar.w*horizPercentage);
			coords[start+i].s = (bar.tx + tX[i]*bar.w*horizPercentage)/width;
		}
	else
		for(int i = 0; i < 6; ++i)
		{
			coords[start+i].x = internalW + flipFactor*(bar.x + gScale*bar.w * (1 - tX[i]*horizPercentage));
			coords[start+i].s = (bar.tx + bar.w*(1 - tX[i]*horizPercentage))/width;
		}

	vao.UpdateBuffer(bar.id, &coords[start], sizeof(Coord)*6, renderer.CurrentFrame());
}

void Hud::SetMatrix(const glm::mat4 &matrix)
{
	pushConstants.transform = matrix;
}