#include "pack_image.h"
#include <image.h>
#include <geometry.h>

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

bool IsPixelTrans(const ImageData &im, uint32_t x, uint32_t y)
{
	uint32_t mul = im.bytesPerPixel;
	if(mul != 1)
	{
		std::cerr << __func__ << ": Unhandled number of channels.\n";
		abort();
	}
	if(mul == 1)
	{
		return im.data[y*im.width*mul + x*mul] == 0;
	}
	return false;
}

bool IsChunkTrans(const ImageData &im, uint32_t xStart, uint32_t yStart, uint32_t chunkSize)
{
	for(unsigned int y = yStart; y < chunkSize+yStart; y++){
		for(unsigned int x = xStart; x < chunkSize+xStart; x++){
			if(!IsPixelTrans(im,x,y)){
				return false;
			}
		}
	}
	return true;
}

Rect GetImageBoundaries(const ImageData &im)
{
	int bottom = -1, top = 0, left = 0, right = 0;

	//Find bottom.
	for(int y = 0; y < (int)im.height; y++){
		for(int x = 0; x < (int)im.width; x++){
			if(!IsPixelTrans(im,x,y)){
				bottom = y;
				goto bottom_end;
			}
		}
	}
	if(bottom == -1)
	{
		std::cout << __func__ << ": The image is empty.";
		return Rect();
	}
	bottom_end:

	//Find top.
	for(int y = im.height-1; y >= bottom; y--){
		for(int x = 0; x < (int)im.width; x++){
			if(!IsPixelTrans(im,x,y)){
				top = y+1;
				goto top_end;
			}
		}
	}
	top_end:

	//Find left.
	for(int x = 0; x < (int)im.width; x++){
		for(int y = bottom; y < top; y++){
			if(!IsPixelTrans(im,x,y)){
				left = x;
				goto left_end;
			}
		}
	}
	left_end:

	//Find right;
	for(int x = im.width-1; x >= left; x--){
		for(int y = bottom; y < top; y++){
			if(!IsPixelTrans(im,x,y)){
				right = x+1;
				goto right_end;
			}
		}
	}
	right_end:

	return Rect(left, bottom, right, top);
}

bool CopyChunk(ImageData &dst, const ImageData &src, 
uint32_t xDst, uint32_t yDst, uint32_t xSrc, uint32_t ySrc, uint32_t chunkSize)
{
	if(dst.bytesPerPixel != src.bytesPerPixel)
	{
		std::cerr << __func__ << ": Number of channels differ.";
		return false;
	}

	if(	xDst+chunkSize > dst.width || yDst+chunkSize > dst.height
	||	xSrc+chunkSize > src.width || ySrc+chunkSize > src.height
	||	xDst < 0 || yDst < 0 || xSrc < 0 || ySrc < 0 
	)
	{
		std::cerr << __func__ << ": Trying to access chunk data out of boundaries.";
		return false;
	}

	int mul = dst.bytesPerPixel;
	for(unsigned int row = 0; row < chunkSize; row++)
	{
		//Y + X + row, chunkSize
		memcpy(
			dst.data + (yDst+row)*dst.width*mul + xDst*mul,
			src.data + (ySrc+row)*src.width*mul + xSrc*mul,
			chunkSize
		);
	}

	return true; 
}

void WriteVertexData(std::string filename, int nChunks, std::list<ImageMeta> &metas, const ImageData &atlas, int chunkSize)
{
	std::ofstream vertexFile(filename, std::ios_base::binary);
	vertexFile.write((char*)&nChunks, sizeof(int));
	auto data = new VertexData8[nChunks*6];
	int dataI = 0;
	constexpr float tX[] = {0,1,1, 1,0,0};
	constexpr float tY[] = {0,0,1, 1,1,0};
	for(auto &meta: metas)
	{
		for(auto &chunk: meta.chunks)
		{
			for(int i = 0; i < 6; i++)
			{
				data[dataI+i].x = chunk.x + chunkSize*tX[i];
				data[dataI+i].y = chunk.y + chunkSize*tY[i];

				data[dataI+i].s = (chunk.x + chunkSize*tX[i])/atlas.width;
				data[dataI+i].t = (chunk.y + chunkSize*tY[i])/atlas.height;

				data[dataI+i].atlasId = meta.atlasId;
			}
			dataI += 6;
		}
	}
	vertexFile.write((char*)data, nChunks*6*sizeof(VertexData8));
	delete[] data;
}

int main()
{
	namespace fs = std::filesystem;

	int chunkSize = 8;
	int nChunks = 0;

	std::string folderIn = "data/images/src/";

	std::list<ImageMeta> images8bpp;
	for(const fs::directory_entry &file : fs::directory_iterator(folderIn))
	{
		ImageMeta im {
			std::make_unique<ImageData>(file.path().generic_string().c_str()),
			file.path().filename().string()
		};
		if(im.data->data)
		{
			if(im.data->bytesPerPixel == 1)
			{
				im.CalculateChunks(chunkSize);
				nChunks += im.chunks.size();
				images8bpp.push_back(std::move(im));
			}
			else 
			{
				//TODO: Handle 32bpp images
			}
		}
		
	}

	

	//Calculate size of atlas.
	int atlasChunks = nChunks/4 + !!(nChunks%4);

	std::cout << "Read " << images8bpp.size() << " images.\n"
		<< nChunks << " chunks required ("<<atlasChunks<<" per channel)\n";


	std::vector<std::unique_ptr<Atlas>> atlases;
	atlases.reserve(4);
	for(int i = 0; i < 4; i++)
	{
		atlases.push_back(std::make_unique<Atlas>(1024, 1024, 1, chunkSize, i));
		memset(atlases[i]->image.data, 255, atlases[i]->image.GetMemSize());
	}

	auto atlasI = atlases.begin();
	for(auto &im: images8bpp)
	{
		if(!(*atlasI)->CopyToAtlas(im))
		{
			atlasI++;
			if(atlasI == atlases.end())
			{
				std::cout << "Not enough atlases.\n";
				break;
			}
			//Copy the image that couldn't be copied.
			(*atlasI)->CopyToAtlas(im);
		}
	}
	
	ImageData composite(1024, 1024, 4);
	for(int y = 0; y < 1024; y++)
	{
		for(int x = 0; x < 1024; x++)
		{
			for(int ch = 0; ch < 4; ch++)
			{
				ImageData &srcIm = atlases[ch]->image;
				composite.data[y*composite.width*composite.bytesPerPixel + x*composite.bytesPerPixel + ch] =
					srcIm.data[y*srcIm.width*srcIm.bytesPerPixel + x*srcIm.bytesPerPixel];
			}
		}
	}

	std::cout << "Writing file... ";
	composite.WriteAsPng("vaki.png");
	std::cout << "Done\n";

	WriteVertexData("vaki.vt8", nChunks, images8bpp, composite, chunkSize);


	return 0;
}

