#include "vao.h"
#include <glad/glad.h>
#include <iostream>

Vao::Vao(AttribType _type, unsigned int _usage):
type(_type), usage(_usage), loaded(false), stride(0), totalSize(0), vaoId(0), vboId(0)
{
	glGenVertexArrays(1, &vaoId);
	switch(type)
	{
	case F2F2:
		stride = 4 * sizeof(float) ;
		break;
	case F2F2I1:
		stride = 4 * sizeof(float) + sizeof(int);
		break;
	}
}

Vao::~Vao()
{
	glDeleteVertexArrays(1, &vaoId);
	glDeleteBuffers(1, &vboId);
}

int Vao::Prepare(size_t size, void *ptr)
{
	if(int alignment = size % stride)
	{
		std::cerr << __FUNCTION__ << ": Size "<<size<<" is not a multiple of the stride "<<stride<<".\n";
		size -= alignment;
	}

	dataPointers.push_back(memPtr{
		(uint8_t*) ptr,
		size,
		totalSize/stride
	});
	totalSize += size;
	return dataPointers.size() - 1;
}

void Vao::Draw(int which, size_t count, int mode)
{
	if(count == 0)
		count = dataPointers[which].size/stride;
	glDrawArrays(mode, dataPointers[which].location, count);
}

void Vao::UpdateBuffer(int which, void *data, size_t count)
{
	if(count == 0)
		count = dataPointers[which].size;

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferSubData(GL_ARRAY_BUFFER, dataPointers[which].location*stride, count, data);

}

void Vao::Bind()
{
	glBindVertexArray(vaoId);
}

void Vao::Load()
{
	Bind();
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, usage);

	uint8_t *data = (uint8_t*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	size_t where = 0;
	for(auto &subData : dataPointers)
	{
		if(subData.ptr)
			memcpy(data+where, subData.ptr, subData.size);
		where += subData.size;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	switch(type)
	{
	case F2F2I1:
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*2));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, stride, (void*)(sizeof(float)*4));
		glEnableVertexAttribArray(2);
		break;
	case F2F2:
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*2));
		glEnableVertexAttribArray(1);
		break;
	}

}