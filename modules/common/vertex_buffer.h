#ifndef VERTEX_BUFFER_H_GUARD
#define VERTEX_BUFFER_H_GUARD

#include <vector>
#include <cstdint>
#include <utility> //pair
#include "vk/renderer.h"

//More like builder/indexer, but w/e
class VertexBuffer
{
private:
	Renderer &renderer;
	bool loaded;

	struct memPtr
	{
		uint8_t *ptr;
		size_t size;
		size_t location;
		size_t stride;
	};

	
	size_t totalSize = 0;
	size_t eboSize = 0;
	std::vector<memPtr> dataPointers;

public:
	AllocatedBuffer buffer;
	
	VertexBuffer(Renderer*);
	~VertexBuffer();

	//Returns index of object that can be drawn.
	int Prepare(size_t size, unsigned int stride, void *ptr);
	std::pair<size_t,size_t> Index(int which) const;
	void UpdateBuffer(int which, void *data, size_t count = 0);
	void UpdateElementBuffer(void *data, size_t count);
	void Bind();
	void Load();
	
};

#endif /* VERTEX_BUFFER_H_GUARD */