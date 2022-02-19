#include "pipeline_builder.h"
#include "renderer.h"
#include <fstream>
#include "format_info.h"
#include <spirv_reflect.h>

#define spvr_assert(x) assert(x == SPV_REFLECT_RESULT_SUCCESS)

PipelineBuilder::~PipelineBuilder(){}
PipelineBuilder::PipelineBuilder(vk::raii::Device *device, Renderer* renderer):
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
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha, // Optional
		.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha, // Optional
		.colorBlendOp = vk::BlendOp::eAdd, // Optional
		.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha, // Optional
		.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha, // Optional
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
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
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

PipelineBuilder& PipelineBuilder::SetSpecializationConstants(const std::initializer_list<int32_t> &values)
{
	specValues = values;
	specEntries.resize(values.size());
	for(unsigned int i = 0; i < values.size(); ++i)
	{
		specEntries[i] = {i, i*(uint32_t)sizeof(int32_t), sizeof(int32_t)};
	}
	specInfo = {
		(uint32_t)specEntries.size(),
		specEntries.data(),
		specEntries.size()*sizeof(uint32_t),
		&specValues
	};
	return *this;
}

PipelineBuilder& PipelineBuilder::SetShaders(path vertex, path fragment)
{
	shaderStages.clear();
	shaderModules.clear();
	path shaders[]{vertex, fragment};
	vk::ShaderStageFlagBits shaderStageFlags[]{
		vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eFragment
	};

	vk::SpecializationInfo *specialization = nullptr;
	if(!specEntries.empty())
		specialization = &specInfo;

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
		shaderStages.push_back(vk::PipelineShaderStageCreateInfo{
			.stage = shaderStageFlags[i],
			.module = *shaderModules.back(),
			.pName = "main",
			.pSpecializationInfo = specialization,
		});

		BuildDescriptorSetsBindings(createInfo.pCode, createInfo.codeSize, shaderStageFlags[i]);
	}

	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.stageCount = shaderStages.size();

	return *this;
}

void PipelineBuilder::BuildDescriptorSetsBindings(const void* code, size_t size, vk::ShaderStageFlagBits stage)
{
	SpvReflectShaderModule spvModule;
	uint32_t count = 0;
	spvr_assert( spvReflectCreateShaderModule(size, code, &spvModule) );

	//Bindings
/* 	spvr_assert( spvReflectEnumerateDescriptorBindings(&module, &count, nullptr) );
	std::vector<SpvReflectDescriptorBinding*> bindings(count);
	spvr_assert( spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data()) ); 

 	spvr_assert( spvReflectEnumerateSpecializationConstants(&spvModule, &count, nullptr) );
	std::vector<SpvReflectSpecializationConstant*> spvConstants(count);
	spvr_assert( spvReflectEnumerateSpecializationConstants(&spvModule, &count, spvConstants.data()) );
 */
	spvr_assert( spvReflectEnumerateDescriptorSets(&spvModule, &count, nullptr) );
	std::vector<SpvReflectDescriptorSet*> spvSets(count);
	spvr_assert( spvReflectEnumerateDescriptorSets(&spvModule, &count, spvSets.data()) );
	for(auto &spvSet: spvSets)
	{
		auto set = spvSet->set;
		assert(set >= 0 && set <= maxSets);
		for(uint32_t i = 0; i < spvSet->binding_count; ++i)
		{
			auto &binding = *spvSet->bindings[i];
			auto it = setBindings[set].find(binding.binding);
			if(it != setBindings[set].end())
			{
				it->second.stageFlags |= stage;
			}
			else
			{
				uint32_t count = binding.count;
				if(binding.count == 0xFFFFFFFF) //specialization constant
				{
					auto loc = binding.type_description->traits.array.spec_constant_op_ids[0];
					SpvReflectResult result;
					auto thing = spvReflectGetSpecializationConstantByLocation(&spvModule, loc, &result);
					assert(result == SPV_REFLECT_RESULT_SUCCESS);
					assert(specValues.size() > thing->constant_id);
					count = specValues[thing->constant_id];
				}
				setBindings[set].emplace(binding.binding, vk::DescriptorSetLayoutBinding{
					.binding = binding.binding,
					.descriptorType = (vk::DescriptorType)binding.descriptor_type,
					.descriptorCount = count,
					.stageFlags = stage
				});
			}
		}
	}
	spvReflectDestroyShaderModule(&spvModule);
}

PipelineBuilder& PipelineBuilder::SetInputLayout(bool interleaved, std::initializer_list<vk::Format> formats)
{
	viBindings.clear();
	viAttributes.clear();
	if(interleaved)
	{
		uint32_t stride = 0;
		for(const auto &format : formats)
		{	
			auto info = FormatInfo::GetFormatInfo(format);
			stride += info.bytes;
		}

		vk::VertexInputBindingDescription binding = {};
		binding.binding = 0;
		binding.stride = stride;
		binding.inputRate = vk::VertexInputRate::eVertex;

		viBindings.push_back(binding);

		uint32_t offset = 0, location = 0;
		for(const auto &format : formats)
		{
			auto info = FormatInfo::GetFormatInfo(format);
			vk::VertexInputAttributeDescription attribute = {
				.location = location,
				.binding = 0,
				.format = format,
				.offset = offset,
			};

			offset+= info.bytes;
			location++;

			viAttributes.push_back(attribute);
		}
	}
	else{
		assert(0 && "Unimplemented");
	}

	return *this;
}

PipelineBuilder& PipelineBuilder::SetPushConstants(std::initializer_list<vk::PushConstantRange> ranges)
{
	pushConstantRanges = std::move(ranges);
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
	return *this;
}

void PipelineBuilder::Build(vk::raii::Pipeline &pipeline, vk::raii::PipelineLayout &pLayout,
	std::vector<vk::DescriptorSet> &sets, std::vector<vk::raii::DescriptorSetLayout> &setLayouts)
{
	std::vector<vk::DescriptorSetLayout> createSetWLayout;
	for(int i = 0; i < maxSets; ++i)
	{
		if(setBindings[i].empty())
			continue;
		
		std::vector<vk::DescriptorSetLayoutBinding> bindings; bindings.reserve(setBindings[i].size());
		for(auto &bindingI : setBindings[i])
			bindings.push_back(bindingI.second);

		vk::DescriptorSetLayoutCreateInfo setInfo = {
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};
		
		setLayouts.emplace_back(*device, setInfo);
		createSetWLayout.push_back(*setLayouts.back());
	}
	
	//if(sets.empty())
		sets = renderer->CreateDescriptorSets(createSetWLayout);
	/* else
	{
		auto newSets = renderer->CreateDescriptorSets(createSetWLayout);
		sets.reserve(sets.size()+newSets.size());
		std::move(std::begin(newSets), std::end(newSets), std::back_inserter(sets));
	} */

	setsCached = sets;
	assert(setsCached.size() <= 4);

	vertexInputInfo.pVertexAttributeDescriptions = viAttributes.data();
	vertexInputInfo.vertexAttributeDescriptionCount = viAttributes.size();
	vertexInputInfo.pVertexBindingDescriptions = viBindings.data();
	vertexInputInfo.vertexBindingDescriptionCount = viBindings.size();
	pipelineLayoutInfo.setLayoutCount = createSetWLayout.size();
	pipelineLayoutInfo.pSetLayouts = createSetWLayout.data();

	std::tie(pipeline, pLayout) = renderer->RegisterPipelines(pipelineInfo, pipelineLayoutInfo);
}

void PipelineBuilder::UpdateSets(const std::vector<WriteSetInfo> &parameters)
{
	std::vector<VkWriteDescriptorSet> setWriteInfos; setWriteInfos.reserve(parameters.size());
	std::vector<VkDescriptorBufferInfo> bufferArr; bufferArr.reserve(parameters.size());
	std::vector<VkDescriptorImageInfo> imageArr; imageArr.reserve(parameters.size());
	std::unordered_map<int,int> setAndBindCount[4];

	for(const auto &param: parameters)
	{
		auto &currSet = setAndBindCount[param.set];
		auto it = currSet.find(param.binding);
		if(it != currSet.end())
		{
			it->second += 1;
		}
		else
			currSet.emplace(param.binding, 1);
	}

	for(int pI = 0; pI < parameters.size(); ++pI)
	{
		auto &param = parameters[pI];
		auto &currentBinding = setBindings[param.set][param.binding];

		VkWriteDescriptorSet write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = setsCached[param.set],
			.dstBinding = (unsigned)param.binding,
			.descriptorCount = 1,
			.descriptorType = (VkDescriptorType)currentBinding.descriptorType
		};

		auto count = setAndBindCount[param.set][param.binding];
		if(count > 1)	
		{
			assert(count <= write.descriptorCount);
			write.descriptorCount = count;
			if(param.type == 0)
			{
				write.pBufferInfo = bufferArr.data()+bufferArr.size();
				for(int i = 0; i < count && i < parameters.size() - pI; ++i, ++pI)
				{
					assert(parameters[pI].type == 0);
					bufferArr.push_back(parameters[pI].data.buffer);
				}
			}
			else //if(param.type == 1)
			{
				write.pImageInfo = imageArr.data()+imageArr.size();
				for(int i = 0; i < count && i < parameters.size() - pI; ++i, ++pI)
				{
					assert(parameters[pI].type == 1);
					imageArr.push_back(parameters[pI].data.image);	
				}
			}
		}
		else if(param.type == 0)
		{
			write.pBufferInfo = &param.data.buffer;
		}
		else //if(param.type == 1)
		{
			write.pImageInfo = &param.data.image;
		}

		setWriteInfos.push_back(write);
	}

	vkUpdateDescriptorSets(**device, setWriteInfos.size(), setWriteInfos.data(), 0, nullptr);
}