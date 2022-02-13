#ifndef PIPELINE_BUILDER_H_GUARD
#define PIPELINE_BUILDER_H_GUARD

#include <vulkan/vulkan_raii.hpp>
#include <tuple>
#include <filesystem>

class Renderer;
class Pipeline
{
private:
	using path = std::filesystem::path;
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vk::raii::ShaderModule> shaderModules;
	vk::raii::Device* device;
	Renderer* renderer;
public:
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	vk::Viewport viewport;
	vk::Rect2D scissor;
	vk::PipelineViewportStateCreateInfo viewportState;
	vk::PipelineRasterizationStateCreateInfo rasterizer;
	vk::PipelineMultisampleStateCreateInfo multisampling;
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	vk::PipelineDynamicStateCreateInfo dynamics;
	std::vector<vk::DynamicState> dynStates;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	vk::GraphicsPipelineCreateInfo pipelineInfo;

	Pipeline(vk::raii::Device *device, Renderer* renderer);
	
	Pipeline& SetShaders(path vertex, path fragment);
	int Build();
	
};

#endif /* PIPELINE_BUILDER_H_GUARD */
