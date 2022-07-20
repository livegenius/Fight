#ifndef PIPESET_HPP_GUARD
#define PIPESET_HPP_GUARD

#include <vulkan/vulkan_raii.hpp>

struct PipeSet
{
	unsigned maxSetIndex;
	unsigned setIndices[4];
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	std::vector<vk::raii::DescriptorSetLayout> setLayouts;
	std::vector<vk::DescriptorSet> sets;

	const vk::DescriptorSet &GetSet(unsigned set, unsigned which) const;
};

#endif /* PIPESET_HPP_GUARD */
