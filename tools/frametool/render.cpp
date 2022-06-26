#include "render.h"

#include <iostream>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <gfx_handler.h>
#include <hitbox_renderer.h>
#include <imgui.h>

namespace fs = std::filesystem;

constexpr int maxBoxes = 33;

struct VertexData8
{
	unsigned short x,y,s,t;
	unsigned short atlasId;
};

Render::Render(Renderer* renderer):
gfx(new GfxHandler(renderer)),
hr(new HitboxRenderer(*renderer)),
colorRgba{1,1,1,1},
spriteId(-1),
x(0), offsetX(0),
y(0), offsetY(0),
rotX(0), rotY(0), rotZ(0)
{}

Render::~Render(){}

void Render::LoadGraphics(fs::path path)
{
	gfx->LoadGfxFromDef(path);
	gfx->LoadingDone();
}

void Render::Draw()
{
	//Axis
	glm::mat4 hPView = glm::scale(glm::mat4(1.f), glm::vec3(scale, scale, 1.f));
	hPView = projection*glm::translate(hPView, glm::vec3(x,y,0.f));
	hr->DrawAxisOnly(hPView, 0.25f);

	//Sprite
	constexpr float tau = glm::pi<float>()*2.f;
	glm::mat4 sView = glm::mat4(1.f);
	sView = glm::scale(sView, glm::vec3(scale, scale, 1.f));
	sView = glm::translate(sView, glm::vec3(x,y,0.f));
 	sView = glm::scale(sView, glm::vec3(scaleX,scaleY,0));
	sView = glm::rotate(sView, -rotZ*tau, glm::vec3(0.0, 0.f, 1.f));
	sView = glm::rotate(sView, rotY*tau, glm::vec3(0.0, 1.f, 0.f));
	sView = glm::rotate(sView, rotX*tau, glm::vec3(1.0, 0.f, 0.f));
	sView = glm::translate(sView, glm::vec3(offsetX, offsetY,0.f)); 

	gfx->Begin();
	gfx->SetMatrix(projection*sView);
	gfx->Draw(spriteId);

	//Boxes
	hr->Draw(hPView);
}

void Render::UpdateProj(float w, float h)
{
	projection = glm::ortho<float>(0, w, h, 0, -1024.f, 1024.f);
}

void Render::DontDraw()
{
	hr->DontDraw();
}

void Render::SetImageColor(float *rgba)
{
	if(rgba)
	{
		for(int i = 0; i < 4; i++)
			colorRgba[i] = rgba[i];
	}
	else
	{
		for(int i = 0; i < 4; i++)
			colorRgba[i] = 1.f;
	}
}

void Render::GenerateHitboxVertices(const std::vector<int> &boxes, int color)
{
	std::vector<float> boxesF(boxes.size());
	for(size_t i = 0; i < boxes.size(); ++i)
		boxesF[i] = boxes[i];
	hr->GenerateHitboxVertices(boxesF, color);
}

void Render::LoadHitboxVertices()
{
	hr->LoadHitboxVertices();
}