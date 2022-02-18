#ifndef PIPELINE_BUILDER_H_GUARD
#define PIPELINE_BUILDER_H_GUARD

#include <vulkan/vulkan_raii.hpp>
#include <unordered_map>
#include <tuple>
#include <filesystem>


class Renderer;
class PipelineBuilder
{
public:
	struct WriteSetInfo{
		union {
			VkDescriptorBufferInfo buffer; //0
			VkDescriptorImageInfo image; //1
		} data; // access with some_info_object.data.a or some_info_object.data.b
		int type;
		int set; int binding;
		WriteSetInfo(vk::DescriptorBufferInfo buffer, int set, int binding): type(0), set(set), binding(binding){ data.buffer = buffer; }
		WriteSetInfo(vk::DescriptorImageInfo image, int set, int binding): type(1), set(set), binding(binding){ data.image = image; }
	};

private:
	static int constexpr maxSets = 4;
	using path = std::filesystem::path;
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vk::raii::ShaderModule> shaderModules;
	std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> setBindings[maxSets];
	const vk::raii::Device* device;
	Renderer* renderer;
	std::vector<vk::DescriptorSet> setsCached;

	void BuildDescriptorSetsBindings(const void* code, size_t size, vk::ShaderStageFlagBits);
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

	std::vector<vk::VertexInputBindingDescription> viBindings;
	std::vector<vk::VertexInputAttributeDescription> viAttributes;

	std::vector<vk::PushConstantRange> pushConstantRanges;

	PipelineBuilder(vk::raii::Device *device, Renderer* renderer);
	~PipelineBuilder();
	
	PipelineBuilder& SetShaders(path vertex, path fragment);
	PipelineBuilder& SetInputLayout(bool interleaved, std::initializer_list<vk::Format> formats);
	PipelineBuilder& SetPushConstants(std::initializer_list<vk::PushConstantRange> ranges);
	void Build(vk::raii::Pipeline &pipeline, vk::raii::PipelineLayout &pLayout,
		std::vector<vk::DescriptorSet> &sets, std::vector<vk::raii::DescriptorSetLayout> &setLayouts);
	void UpdateSets(const std::vector<WriteSetInfo> &parameters);
};

#endif /* PIPELINE_BUILDER_H_GUARD */
