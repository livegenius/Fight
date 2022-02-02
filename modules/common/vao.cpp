#include "vao.h"
#include <iostream>
#include <cstring>

Vao::Vao(AttribType _type, unsigned int _usage, size_t eboSize):
type(_type), usage(_usage), loaded(false), stride(0), totalSize(0), eboSize(eboSize),
vaoId(0), vboId(0), eboId(0)
{
	/* glGenVertexArrays(1, &vaoId);
	switch(type)
	{
	case F2F2:
		stride = 4 * sizeof(float);
		break;
	case F2F2_short:
		stride = 4 * sizeof(short);
		break;
	case F3F3:
		stride = 6 * sizeof(float);
		break;
	}

	if(eboSize)
	{
		glGenBuffers(1, &eboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboSize, nullptr, _usage); 
	} */
}

Vao::~Vao()
{
/* 	glDeleteVertexArrays(1, &vaoId);
	glDeleteBuffers(1, &vboId);
	glDeleteBuffers(1, &eboId); */
}

int Vao::Prepare(size_t size, void *ptr)
{
/* 	if(int alignment = size % stride)
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
	return dataPointers.size() - 1; */
	return 0;
}

void Vao::Draw(int which, int mode)
{
/* 	size_t count = dataPointers[which].size/stride;
	glDrawArrays(mode, dataPointers[which].location, count); */
}

void Vao::DrawCount(int which, int count, int mode)
{
//	glDrawArrays(mode, dataPointers[which].location, count);
}

void Vao::DrawInstances(int which, size_t instances, int mode)
{
	int vCount = dataPointers[which].size/stride;
//	glDrawArraysInstanced(mode, dataPointers[which].location, vCount, instances);
}


void Vao::UpdateBuffer(int which, void *data, size_t count)
{
	if(count == 0)
		count = dataPointers[which].size;

/* 	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferSubData(GL_ARRAY_BUFFER, dataPointers[which].location*stride, count, data); */
}

void Vao::UpdateElementBuffer(void *data, size_t count)
{
	if(count > eboSize)
		count = eboSize;
/* 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count, data); */
}

void Vao::Bind()
{
/* 	glBindVertexArray(vaoId);
	if(eboId)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId); */
}

void Vao::Load()
{
	Bind();
	/* glGenBuffers(1, &vboId);
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
	case F2F2_short:
		glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, stride, nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, stride, (void*)(2*sizeof(short)));
		glEnableVertexAttribArray(1);
		break;
	case F3F3:
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*3));
		glEnableVertexAttribArray(1);
		break;
	case F2F2:
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*2));
		glEnableVertexAttribArray(1);
		break;
	} */

}
