#ifndef PIPELINE_BUILDER_H_GUARD
#define PIPELINE_BUILDER_H_GUARD

#include <vulkan/vulkan_raii.hpp>
#include <tuple>
#include <filesystem>

class Renderer;
class PipelineBuilder
{
private:
	using path = std::filesystem::path;
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vk::raii::ShaderModule> shaderModules;
	vk::raii::Device* device;
	Renderer* renderer;

	void Reflection(const void* code, size_t size);
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

	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;

	PipelineBuilder(vk::raii::Device *device, Renderer* renderer);
	
	PipelineBuilder& SetShaders(path vertex, path fragment);
	PipelineBuilder& SetInputLayout(bool interleaved, std::initializer_list<vk::Format> formats);
	int Build();
	
};

#endif /* PIPELINE_BUILDER_H_GUARD */
