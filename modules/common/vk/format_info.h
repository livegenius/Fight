#ifndef VK_FORMAT_UTILS_H_GUARD
#define VK_FORMAT_UTILS_H_GUARD

#include <vulkan/vulkan.h>

struct FormatInfo {
	int bytes;
	int componentCount;
	struct{
		int type;
		int size; // bits
	} components[4];

	static FormatInfo GetFormatInfo(const VkFormat format);
	static FormatInfo GetFormatInfo(const vk::Format format){return GetFormatInfo((VkFormat)format);}

	enum {
		NONE,
		R,
		G,
		B,
		A,
		D,
		S
	};
};


#endif /* VK_FORMAT_UTILS_H_GUARD */

