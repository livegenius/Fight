#include "hitbox_renderer.h"
#include <vk/renderer.h>

constexpr size_t maxBoxes = 128;
constexpr size_t vertexBufferSize = sizeof(float)*6*4*maxBoxes + 6*4*sizeof(float);
constexpr size_t indexBufferSize = sizeof(uint16_t)*(6+8)*maxBoxes + 4*sizeof(uint16_t);
static_assert(4*maxBoxes <= 0xFFFFu, "16 bit element buffer can't hold all box vertices.");

HitboxRenderer::HitboxRenderer(Renderer &renderer):
renderer(renderer)
{
	vertices.Allocate(&renderer, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eCpuToGpu);
	indices.Allocate(&renderer, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vma::MemoryUsage::eGpuOnly);

	{ //Generate element buffer indices. They're static.
		AllocatedBuffer stagingBuffer(&renderer, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
		uint16_t *data = (uint16_t*)stagingBuffer.Map();

		//Filling
		constexpr int triangleIndexOffset[6] = {0,1,2,2,3,0};
		for(size_t i = 0, ti = 0; i < 6*maxBoxes; i+=6, ti+=4)
		{
			for(unsigned offset = 0; offset < 6; ++offset)
				data[i+offset] = triangleIndexOffset[offset] + ti;
		}

		//Outlines
		constexpr int lineIndexOffset[8] = {0,1,1,2,2,3,3,0};
		for(size_t i = 6*maxBoxes, li = 0; i < (6+8)*maxBoxes; i+=8, li+=4)
		{
			for(unsigned offset = 0; offset < 8; ++offset)
				data[i+offset] = lineIndexOffset[offset] + li;
		}

		//Axis indices.
		auto offset = (6+8)*maxBoxes;
		for(size_t i = 0; i < 4; ++i)
			data[offset+i] = 4*maxBoxes+i;

		renderer.TransferBuffer(stagingBuffer, indices, indexBufferSize);
	}

	//Axis vertices
	float axisData[]
	{
		-100000, 0, 1,	1,1,1,
		100000, 0, 1,	1,1,1,
		0, 100000, 1,	1,1,1,
		0, -100000, 1,	1,1,1,
	};
	float *quadVertices = (float *)vertices.Map();
	memcpy(&quadVertices[6*4*maxBoxes], axisData, sizeof(axisData));
	vertices.Unmap();

	auto pBuilder = renderer.GetPipelineBuilder();
	pBuilder
		.SetShaders("data/spirv/box.vert.bin", "data/spirv/box.frag.bin")
		.SetInputLayout(true, {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat})
		.SetPushConstants({
			{.stageFlags = vk::ShaderStageFlagBits::eVertex, .size = sizeof(PushConstants)},
		})
	;
	auto &blend = pBuilder.colorBlendAttachment;
	blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;

	pBuilder.inputAssembly.topology = vk::PrimitiveTopology::eLineList;
	lines = pBuilder.Build(pipeset);
	auto &ds = pBuilder.depthStencil;
	ds.depthTestEnable = VK_TRUE;
	ds.depthCompareOp = vk::CompareOp::eNotEqual;
	ds.depthWriteEnable = VK_TRUE;
	pBuilder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	filling = pBuilder.BuildDerivate();
}

void HitboxRenderer::Draw(const glm::mat4 &transform)
{
	if(quadsToDraw > 0)
	{
		PushConstants pushConstants {transform, 0.1f};

		auto cmd = renderer.GetCommand();
		cmd->bindVertexBuffers(0, vertices.buffer, {0});
		cmd->bindIndexBuffer(indices.buffer, 0, vk::IndexType::eUint16);

		cmd->pushConstants(*pipeset.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstants), &pushConstants);
		cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *filling);
		cmd->drawIndexed(quadsToDraw*6, 1, 0, 0, 0);
		
		pushConstants.alpha = 1.0;
		cmd->pushConstants(*pipeset.layout, vk::ShaderStageFlagBits::eVertex, offsetof(PushConstants, alpha), sizeof(float), &pushConstants.alpha);
		cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *lines);
		cmd->drawIndexed(quadsToDraw*8, 1, 6*maxBoxes, 0, 0);
	}
}

void HitboxRenderer::DrawAxisOnly(const glm::mat4 &transform, float alpha)
{
	PushConstants pushConstants {transform, alpha};
	auto cmd = renderer.GetCommand();
	cmd->bindVertexBuffers(0, vertices.buffer, {0});
	cmd->bindIndexBuffer(indices.buffer, 0, vk::IndexType::eUint16);
	cmd->pushConstants(*pipeset.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstants), &pushConstants);
	cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *lines);
	cmd->drawIndexed(4, 1, (8+6)*maxBoxes, 0, 0);
}

void HitboxRenderer::GenerateHitboxVertices(const std::vector<float> &hitboxes, int pickedColor)
{
 	int boxes = hitboxes.size()/4; //Gets BLTR, divide by 4 to get number of individual boxes.
	if(boxes <= 0)
	{
		quadsToDraw = 0;
		return;
	}

	size_t thisBatchFloats = (boxes)*4*6; //4 Vertices with 6 attributes.
	auto endSize = sizeof(float)*(thisBatchFloats + acumFloats);

	if(vertexBufferSize < endSize) 
		endSize = vertexBufferSize; //Do not overflow the buffer
	if(endSize <= sizeof(float)*acumFloats) 
		return; //The buffer is full. Do nothing
	
	//r, g, b, z order
	constexpr float colors[][4]={
		{0.5, 0.5, 0.5, 2},    //gray
		{0.0, 1, 0.0, 4},      //green
		{1, 0.0, 0.0, 8}       //red
	};

	const float * const color = colors[pickedColor];

	constexpr int tX[] = {0,1,1,0};
	constexpr int tY[] = {0,0,1,1};

	float *quadVertices = (float *)vertices.Map();

	int dataI = acumFloats;
	for(int i = 0; i < boxes*4; i+=4) //For every box (2 2d points) in hitboxes[]
	{
		for(int j = 0; j < 4*6; j+=6) //For every of the 4 vertices of a box.
		{
			//X, Y, Z, R, G, B
			quadVertices[dataI+j+0] = hitboxes[i+0] + (hitboxes[i+2]-hitboxes[i+0])*tX[j/6];
			quadVertices[dataI+j+1] = hitboxes[i+1] + (hitboxes[i+3]-hitboxes[i+1])*tY[j/6];
			quadVertices[dataI+j+2] = color[3];
			quadVertices[dataI+j+3] = color[0];
			quadVertices[dataI+j+4] = color[1];
			quadVertices[dataI+j+5] = color[2];
		}
		dataI += 4*6;
	}
	acumFloats = dataI;
}

void HitboxRenderer::LoadHitboxVertices()
{
 	/* vGeometry.UpdateBuffer(geoVaoId, clientQuads.data(), acumQuads*sizeof(float));
	vGeometry.UpdateElementBuffer(clientElements.data(), (acumElements)*sizeof(uint16_t)); */
	quadsToDraw = acumFloats/(4*6);
	acumFloats = 0;
}

void HitboxRenderer::DontDraw()
{
	quadsToDraw = 0;
}