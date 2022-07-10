#ifndef PACK_IMAGE_HPP_GUARD
#define PACK_IMAGE_HPP_GUARD

#include <image.h>
#include <geometry.h>
#include <list>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <algorithm>

typedef Rect2d<int> Rect;
typedef Point2d<int> Point;

Rect GetImageBoundaries(const ImageData &im);
bool IsChunkTrans(const ImageData &im, uint32_t xStart, uint32_t yStart, uint32_t chunkSize);
bool IsPixelTrans(const ImageData &im, uint32_t x, uint32_t y);
bool CopyChunk(ImageData &dst, const ImageData &src, 
	int32_t xDst, int32_t yDst, int32_t xSrc, int32_t ySrc, int32_t chunkSize);
void CalcSizeInChunks(int chunks, int chunkSize, int &width, int &height, bool pow2, bool border);

struct VertexData4
{
	short x,y;
	unsigned short s,t;
};

struct ChunkMeta
{
	Point pos;
	Point tex;
};

struct ImageMeta
{
	std::unique_ptr<ImageData> data;
	std::string name;
	std::list<ChunkMeta> chunks;

	void CalculateChunks(int chunkSize)
	{
		ImageData &image = *data; 
		Rect size = GetImageBoundaries(image);
		//Size of rect in chunks. At least 1.
		auto dif = (size.topRight - size.bottomLeft);
		int wChunks = dif.x/chunkSize + !!(dif.x%chunkSize);
		int hChunks = dif.y/chunkSize + !!(dif.y%chunkSize);

		Rect requiredSize(
			size.bottomLeft.x, size.bottomLeft.y,
			size.bottomLeft.x + wChunks*chunkSize, size.bottomLeft.y + hChunks*chunkSize
		);

		for(int y = requiredSize.bottomLeft.y; y < requiredSize.topRight.y; y += chunkSize)
		{
			for(int x = requiredSize.bottomLeft.x; x < requiredSize.topRight.x; x += chunkSize)
			{
				//TODO: Calculate hash instead.
				if(!IsChunkTrans(image, x, y, chunkSize))
				{
					ChunkMeta chunk{Point(x,y)};
					chunks.push_back(chunk);
				}
			}
		}
	}
};

void WriteVertexData(std::string filename, int nChunks, std::list<ImageMeta> &metas, int chunkSize, float width, float height);


struct Atlas
{
	ImageData image;
	int x, y;
	uint32_t chunkSize;
	int border;

	Atlas(uint32_t width, uint32_t height, uint32_t bytesPerPixel, uint32_t _chunkSize, bool _border = false):
	image(width, height, bytesPerPixel), x(0), y(0), border(_border), chunkSize(_chunkSize)
	{
		chunkSize+=border*2;
	}

	bool Advance()
	{
		if(x + chunkSize > image.width-chunkSize)
		{
			if(y + chunkSize > image.height-chunkSize)
			{
				//The atlas is full.
				return false;
			}
			y += chunkSize;
			x = 0;
		}
		else
			x += chunkSize;
		return true;
	}

	bool CopyToAtlas(ImageMeta &src)
	{
		std::list<ChunkMeta> &chunks = src.chunks;
		for(auto i = chunks.begin(); i!=chunks.end(); i++)
		{
			ChunkMeta &chunk = *i;
			if(!CopyChunk(image, *src.data, x, y, chunk.pos.x-border, chunk.pos.y-border, chunkSize))
			{
				std::cerr << __func__ << " failed.\tyn";
				return false;
			}
			chunk.tex.x = x+border;
			chunk.tex.y = y+border;
			if(!Advance())
			{
				std::cerr << "Atlas filled. Last image: "<<src.name<<".\n";
				return false;
			}
		}
		return true;
	}

	void PremultiplyAlpha()
	{
		if(image.bytesPerPixel != 4)
		{
			std::cerr << "Premultiplied alpha only works for 4 ByPP images!\n";
			return;
		}

		const size_t size = image.GetMemSize();
		for(size_t i = 0; i < size; i+=4)
		{
			float factor = ((float)image.data[i+3])/255.f;
			constexpr float gamma = 2.4f;
			factor = powf(factor, 1.f/gamma);

			image.data[i+0] = static_cast<uint8_t>(std::clamp((static_cast<float>(image.data[i+0])/255.f)*factor, 0.f, 1.f)*255);
			image.data[i+1] = static_cast<uint8_t>(std::clamp((static_cast<float>(image.data[i+1])/255.f)*factor, 0.f, 1.f)*255);
			image.data[i+2] = static_cast<uint8_t>(std::clamp((static_cast<float>(image.data[i+2])/255.f)*factor, 0.f, 1.f)*255);
		}
	}
};

#endif /* PACK_IMAGE_HPP_GUARD */
