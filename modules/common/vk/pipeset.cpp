#include "pipeset.hpp"

const vk::DescriptorSet& PipeSet::GetSet(unsigned set, unsigned which) const
{
	assert(setIndices[set]+which < maxSetIndex);
	return sets[setIndices[set]+which];
}