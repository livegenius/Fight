#include "pipeline.h"
#include "renderer.h"
#include <fstream>

Pipeline::Pipeline(vk::raii::Device *device, Renderer* renderer):
device(device),
renderer(renderer)
{
	vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr, // Optional
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr, // Optional
	};

	inputAssembly = {
		.topology = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = VK_FALSE,
	};

	viewport = vk::Viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = 0.f,
		.height = 0.f,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	scissor = vk::Rect2D{
		.offset = {0, 0},
		.extent = {},
	};

	viewportState = {
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	rasterizer = {
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = vk::PolygonMode::eFill,
		//.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.lineWidth = 1.f,
	};

	multisampling = {
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};

	colorBlendAttachment = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = vk::BlendFactor::eOne, // Optional
		.dstColorBlendFactor = vk::BlendFactor::eZero, // Optional
		.colorBlendOp = vk::BlendOp::eAdd, // Optional
		.srcAlphaBlendFactor = vk::BlendFactor::eOne, // Optional
		.dstAlphaBlendFactor = vk::BlendFactor::eZero, // Optional
		.alphaBlendOp = vk::BlendOp::eAdd, // Optional
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
	};

	colorBlending = {
		.logicOpEnable = VK_FALSE,
		.logicOp = vk::LogicOp::eCopy, // Optional
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = {},
	};

	depthStencil = {
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f, // Optional
		.maxDepthBounds = 1.0f, // Optional
	};

	dynStates = {vk::DynamicState::eViewport , vk::DynamicState::eScissor};
	dynamics = vk::PipelineDynamicStateCreateInfo{
		.dynamicStateCount = (uint32_t)dynStates.size(),
		.pDynamicStates = dynStates.data(),
	};

	pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{
		.setLayoutCount = 0, // Optional
		.pSetLayouts = nullptr, // Optional
		.pushConstantRangeCount = 0, // Optional
		.pPushConstantRanges = nullptr, // Optional
	};

	pipelineInfo = vk::GraphicsPipelineCreateInfo{
		
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil, // Optional
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamics, // Optional
		//.layout = *pipelineLayout,
		
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1, // Optional
	};
}

Pipeline& Pipeline::SetShaders(path vertex, path fragment)
{
	shaderStages.clear();
	shaderModules.clear();
	path shaders[]{vertex, fragment};
	vk::ShaderStageFlagBits shaderStageFlags[]{
		vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eFragment
	};

	for(int i = 0; i < 2; ++i)
	{
		path& shader = shaders[i];
		if(shader.empty())
			continue;
		std::ifstream file(shader, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open shader files!");
		}
		size_t fileSize = (size_t) file.tellg();
		std::vector<uint32_t> buffer((fileSize + sizeof(uint32_t) - 1)/sizeof(uint32_t));
		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		vk::ShaderModuleCreateInfo createInfo{
			.codeSize = buffer.size()*sizeof(uint32_t),
			.pCode = buffer.data(),
		};

		shaderModules.emplace_back(*device, createInfo);
		shaderStages.push_back({
			.stage = shaderStageFlags[i],
			.module = *shaderModules.back(),
			.pName = "main",
		});
	}

	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.stageCount = shaderStages.size();
	return *this;
}

int Pipeline::Build()
{
	return renderer->RegisterPipelines(pipelineInfo, pipelineLayoutInfo);
}

