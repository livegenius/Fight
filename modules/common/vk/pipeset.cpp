#include "pipeset.hpp"

const vk::DescriptorSet& PipeSet::Get(unsigned set, unsigned which) const
{
	assert(setIndices[set]+which < maxSetIndex);
	return sets[setIndices[set]+which];
}