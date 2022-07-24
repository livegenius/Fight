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
	uint8_t *data = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bytesPerPixel = 0;
	bool compressed = false;

	struct Allocator{
		void **ptr = nullptr;
		std::function<void*(size_t)> allocate;
		std::function<void(void**)> deallocate;
	} alloc = {(void**)&data, defaultAllocator, defaultDeallocator};

	ImageData() = default;
	ImageData(Allocator alloc);
	ImageData(uint32_t width, uint32_t height, uint32_t bytesPerPixel);
	ImageData(std::filesystem::path image);
	ImageData(std::filesystem::path image, Allocator alloc);
	~ImageData();
	
	static int PeekBytesPerPixel(std::filesystem::path image);
	//bool PeekMem(void *memory);
	bool LoadAny(std::filesystem::path image);
	bool LoadRaw(std::filesystem::path image);
	bool LoadPng(std::filesystem::path image);
	bool LoadLzs3(std::filesystem::path image);
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
