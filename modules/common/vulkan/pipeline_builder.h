#ifndef PIPELINE_BUILDER_H_GUARD
#define PIPELINE_BUILDER_H_GUARD

#include <vulkan/vulkan_raii.hpp>
#include <tuple>
#include <vector>

class PipelineBuilder
{
	private:
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
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

	PipelineBuilder(vk::RenderPass renderPass = VK_NULL_HANDLE, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {});
	
	void SetShaderStages(std::vector<vk::PipelineShaderStageCreateInfo> shaderStages);
	
	std::pair<vk::raii::PipelineLayout, vk::raii::Pipeline> Build(vk::raii::Device &device);
};

vk::raii::ShaderModule CreateShaderModule(vk::raii::Device &device, const std::string filename);

#endif /* PIPELINE_BUILDER_H_GUARD */
