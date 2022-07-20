#ifndef PIPELINE_BUILDER_H_GUARD
#define PIPELINE_BUILDER_H_GUARD

#include <vulkan/vulkan.hpp>
#include "forward.h"
#include <unordered_map>
#include <tuple>
#include <filesystem>

struct PipeSet;
class Renderer;
class PipelineBuilder
{
public:
	struct WriteSetInfo{
		union {
			VkDescriptorBufferInfo buffer; //0
			VkDescriptorImageInfo image; //1
		} data;
		short type;
		short set; short binding;
		short whichCopy; //In case there are multiple sets with the same layout.
		WriteSetInfo(vk::DescriptorBufferInfo buffer, short set, short binding, short which = 0):
			type(0), set(set), binding(binding), whichCopy(which){ data.buffer = buffer; }
		WriteSetInfo(vk::DescriptorImageInfo image, short set, short binding, short which = 0):
			type(1), set(set), binding(binding), whichCopy(which){ data.image = image; }
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
	std::vector<VkSpecializationMapEntry> specEntries;
	vk::SpecializationInfo specInfo;
	std::vector<int32_t> specValues;
	std::vector<int> actualSetIndices;

	struct intpairhash {
		std::size_t operator()(const std::pair<uint32_t, uint32_t> &x) const{
			return std::hash<uint64_t>()(((uint64_t)x.first << 32) & (uint64_t)x.second);
		}
	};
	std::unordered_map<std::pair<uint32_t, uint32_t>, vk::DescriptorType, intpairhash> SetBindingTypeHints;

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
	
	PipelineBuilder& SetSpecializationConstants(const std::initializer_list<int32_t> &values);
	PipelineBuilder& SetShaders(path vertex, path fragment);
	PipelineBuilder& SetInputLayout(bool interleaved, std::initializer_list<vk::Format> formats);
	PipelineBuilder& SetPushConstants(std::initializer_list<vk::PushConstantRange> ranges);
	PipelineBuilder& HintDescriptorType(uint32_t set, uint32_t binding, vk::DescriptorType type);


	vk::raii::Pipeline Build(PipeSet &pipeset, std::vector<int> numberOfCopies = {});
	vk::raii::Pipeline BuildDerivate();
	
	void UpdateSets(const std::vector<WriteSetInfo> &parameters);
};

#endif /* PIPELINE_BUILDER_H_GUARD */
