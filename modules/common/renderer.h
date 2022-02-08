#ifndef CONTEXT_H_GUARD
#define CONTEXT_H_GUARD

#include <vector>
#include <SDL.h>
#include <filesystem>

constexpr int internalWidth = 480;
constexpr int internalHeight = 270;

class Renderer
{
public:
	static Renderer* NewRenderer();

	virtual bool Init(SDL_Window *, int syncMode) = 0;
	virtual void Submit() = 0;
	virtual bool HandleEvents(SDL_Event) = 0;

	enum TextureType{
		png,
		lzs3,
	};

	struct LoadTextureInfo{
		std::filesystem::path path;
		TextureType type;
		bool repeat = false;
		bool linearFilter = false;
		bool rectangle = false;
	};
	virtual std::vector<int> LoadTextures(std::vector<LoadTextureInfo>&) = 0;

	enum BufferFlags{
		VertexStatic,
		TransferSrc, //Must provide oldBuffer.
	};
	virtual int NewBuffer(size_t size, BufferFlags, int oldBuffer = -1) = 0; 
	virtual void DestroyBuffer(int handle) = 0;
	virtual void* MapBuffer(int handle) = 0;
	virtual void UnmapBuffer(int handle) = 0;
	virtual void TransferBuffer(int src, int dst, size_t size) = 0;
};



#endif /* CONTEXT_H_GUARD */
