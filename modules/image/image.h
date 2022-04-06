#ifndef PNG_WRAPPER_H_GUARD
#define PNG_WRAPPER_H_GUARD
#include <cstdint>
#include <filesystem>
#include <functional>

struct ImageData{
private:
	static void* defaultAllocator(size_t size);
	static void defaultDeallocator(void **);
public:
	struct Allocation{
		void **ptr = nullptr;
		std::function<void*(size_t)> allocate;
		std::function<void(void**)> deallocate;
	};

	uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t bytesPerPixel;
	bool compressed = false;

	ImageData();
	ImageData(uint32_t width, uint32_t height, uint32_t bytesPerPixel);
	ImageData(std::filesystem::path image, Allocation alloc = {nullptr, defaultAllocator, defaultDeallocator});
	~ImageData();
	
	static int PeekBytesPerPixel(std::filesystem::path image);
	//bool PeekMem(void *memory);
	bool LoadAny(std::filesystem::path image, Allocation alloc = {nullptr, defaultAllocator, defaultDeallocator});
	bool LoadRaw(std::filesystem::path image, Allocation alloc = {nullptr, defaultAllocator, defaultDeallocator});
	bool LoadPng(std::filesystem::path image, Allocation alloc = {nullptr, defaultAllocator, defaultDeallocator});
	bool LoadLzs3(std::filesystem::path image, Allocation alloc = {nullptr, defaultAllocator, defaultDeallocator});
	bool WriteRaw(std::filesystem::path path) const;
	std::size_t GetMemSize() const;
	void FreeData();

	enum
	{
		RGBA = 1,
		RGB,
	};
};

#endif /* PNG_WRAPPER_H_GUARD */
