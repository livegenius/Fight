#include "image.h"
#include <spng.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include <lz4.h>

int SpngStreamRead(spng_ctx *ctx, void *user, void *dest, size_t length)
{
	auto *file = (std::ifstream*)user;
	file->read((char *)dest, length);
	if(file->eof())
		return SPNG_IO_EOF;
	else if(file->fail())
		return SPNG_IO_ERROR;
	return 0;
}

void* ImageData::defaultAllocator(size_t size)
{
	return new char[size];
}

void ImageData::defaultDeallocator(void** data)
{
	if(data)
	{
		delete [](char*)*data;
		*data = nullptr;
	}
}

ImageData::ImageData(Allocator alloc_):
alloc(alloc_)
{}

ImageData::ImageData(uint32_t _width, uint32_t _height, uint32_t _bytesPerPixel):
data(nullptr), width(_width), height(_height), bytesPerPixel(_bytesPerPixel)
{
	data = (uint8_t*) malloc(width * height * bytesPerPixel * sizeof(uint8_t));
}

ImageData::ImageData(std::filesystem::path image, Allocator alloc_):
alloc(alloc_)
{
	if(!LoadAny(image))
		throw std::runtime_error(image.string()+" : Can't load image.\n");
}

ImageData::ImageData(std::filesystem::path image): ImageData()
{
	if(!LoadAny(image))
		throw std::runtime_error(image.string()+" : Can't load image.\n");
}

ImageData::~ImageData()
{
	alloc.deallocate(alloc.ptr);
}

bool ImageData::LoadAny(std::filesystem::path image)
{
	if(!LoadLzs3(image) && !LoadPng(image) && !LoadRaw(image))
	{
		std::cerr << image<<": invalid image file\n";
		return false;
	}
	return true;
}

std::size_t ImageData::GetMemSize() const
{
	return width*height*bytesPerPixel;
}

void ImageData::FreeData()
{
	alloc.deallocate(alloc.ptr);
}

struct raw_header{
	uint32_t w, h, bpp;
};

int ImageData::PeekBytesPerPixel(std::filesystem::path path)
{
	std::ifstream image(path, std::ios_base::binary);
	if(image.fail())
		return false;
	
	lzs3:{
		struct{
			uint32_t type;
			uint32_t size;
			uint32_t cSize;
			uint16_t w,h;
		} meta{};
		image.read((char*)&meta, sizeof(meta));
		if((meta.type&~0x1000) > 4 || meta.w > 16384 || meta.h > 16384 || meta.size*2 < meta.cSize) //Checking valid header.
			goto png;
		if(meta.type & 0x1000)
			return meta.type&~0x1000;
		else
			return 4;
	}
	png:{
		image.seekg (0, image.beg);
		std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)> ctxObj(spng_ctx_new(0), &spng_ctx_free);
		auto ctx = ctxObj.get();
		if(ctx == NULL)
			goto raw;

		/* Ignore and don't calculate chunk CRC's */
		spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
		constexpr size_t limit = 4096 * 4096 * 16;
		spng_set_chunk_limits(ctx, limit, limit);
		spng_set_png_stream(ctx, SpngStreamRead, &image);

		struct spng_ihdr ihdr;
		int result = spng_get_ihdr(ctx, &ihdr);

		if(result)
			goto raw;

		switch(ihdr.color_type)
		{
		case SPNG_COLOR_TYPE_GRAYSCALE:
			return 1;
		case SPNG_COLOR_TYPE_TRUECOLOR:
			return 3;
		case SPNG_COLOR_TYPE_INDEXED:
			return 1;
		case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
			return 2;
		default:
			return 4; //RGBA
		}
	}
	raw:{
		image.seekg (0, image.beg);
		raw_header meta;
		image.read((char*)&meta, sizeof(meta));
		if(meta.bpp > 4 || meta.w*meta.h*meta.bpp > 4096 * 4096 * 16)
			goto err;
		return meta.bpp;
	}
	err:
		std::cerr << path <<"is not a valid image file.\n";
		return -1;
}

bool ImageData::LoadPng(std::filesystem::path filename)
{
	FreeData();
	
	std::ifstream png(filename, std::ios_base::binary);
	int r; //result

	if(png.fail())
	{
		std::cerr <<filename<<": error opening input file \n";
		return false;
	}
	
	std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)> ctxObj(spng_ctx_new(0), &spng_ctx_free);
	auto ctx = ctxObj.get();

	if(ctx == NULL)
	{
		std::cerr <<filename<<": spng_ctx_new() failed\n";
		return false;
	}

	/* Ignore and don't calculate chunk CRC's */
	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

	/* Set memory usage limits for storing standard and unknown chunks,
	this is important when reading untrusted files! */
	constexpr size_t limit = 4096 * 4096 * 16;
	spng_set_chunk_limits(ctx, limit, limit);

	/* Set source PNG */
	spng_set_png_stream(ctx, SpngStreamRead, &png);

	struct spng_ihdr ihdr;
	r = spng_get_ihdr(ctx, &ihdr);

	if(r)
	{
		//std::cerr <<filename<<": spng_get_ihdr() error: "<<spng_strerror(r)<<"\n";
		//Probably not a PNG image.
		return false;
	}

	if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE)
		bytesPerPixel = 1;
	else if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR)
		bytesPerPixel = 3;
	else if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
		bytesPerPixel = 1;
	else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA)
		bytesPerPixel = 2;
	else
		bytesPerPixel = 4; //RGBA


	if(ihdr.bit_depth != 8)
	{
		printf("width: %" PRIu32 "\nheight: %" PRIu32 "\n"
			"bit depth: %" PRIu8 "\ncolor type: %" PRIu8 "\n",
			ihdr.width, ihdr.height,
			ihdr.bit_depth, ihdr.color_type);
		printf("compression method: %" PRIu8 "\nfilter method: %" PRIu8 "\n"
			"interlace method: %" PRIu8 "\n",
			ihdr.compression_method, ihdr.filter_method,
			ihdr.interlace_method);
	}

	size_t out_size, out_width;

	/* Output format, does not depend on source PNG format except for
	SPNG_FMT_PNG, which is the PNG's format in host-endian or
	big-endian for SPNG_FMT_RAW.
	Note that for these two formats <8-bit images are left byte-packed */
	int fmt = SPNG_FMT_PNG;

	/* Pick another format to expand them */
	//if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED) fmt = SPNG_FMT_RGB8;

	r = spng_decoded_image_size(ctx, fmt, &out_size);

	if(r) return false;

	

	/* This is required to initialize for progressive decoding */
	r = spng_decode_image(ctx, NULL, 0, fmt, SPNG_DECODE_PROGRESSIVE | SPNG_DECODE_TRNS);
	if(r)
	{
		printf("progressive spng_decode_image() error: %s\n", spng_strerror(r));
		return false;
	}

	/* ihdr.height will always be non-zero if spng_get_ihdr() succeeds */
	out_width = out_size / ihdr.height;
	assert((out_size / ihdr.height)/ihdr.width == bytesPerPixel);

	struct spng_row_info row_info = {0};

	*alloc.ptr = alloc.allocate(out_size);
	if(*alloc.ptr == nullptr) return false;

	do
	{
		r = spng_get_row_info(ctx, &row_info);
		if(r) break;

		//Decode upside down as usual.
		r = spng_decode_row(ctx, (uint8_t*)*alloc.ptr+out_size - out_width - row_info.row_num * out_width, out_width);
	}
	while(!r);

	if(r != SPNG_EOI)
	{
		printf("progressive decode error: %s\n", spng_strerror(r));

		if(ihdr.interlace_method)
			printf("last pass: %d, scanline: %" PRIu32 "\n", row_info.pass, row_info.scanline_idx);
		else
			printf("last row: %" PRIu32 "\n", row_info.row_num);
		alloc.deallocate(alloc.ptr);
		return false;
	}

	/* Alternatively you can decode the image in one go,
	this doesn't require a separate initialization step. */
	/* r = spng_decode_image(ctx, out, out_size, SPNG_FMT_RGBA8, 0);
	if(r)
	{
		printf("spng_decode_image() error: %s\n", spng_strerror(r));
		goto error;
	} */

	width = ihdr.width;
	height = ihdr.height;
	return true;
}


bool ImageData::LoadRaw(std::filesystem::path filename)
{
	FreeData();

	std::ifstream in(filename, std::ios_base::binary);
	if(!in.is_open())
		return false;
	
	raw_header meta;
	in.read((char*)&meta, sizeof(meta));
	int size = meta.w*meta.h*meta.bpp;
	if(size > 4096 * 4096 * 16)
		return false;

	width = meta.w;
	height = meta.h;
	bytesPerPixel = meta.bpp;
	*alloc.ptr = alloc.allocate(size); //(uint8_t*)malloc(size);
	in.read((char*)*alloc.ptr, size);
	if(in.fail())
	{
		alloc.deallocate(alloc.ptr);
		return false;
	}
	return true;
}

bool ImageData::LoadLzs3(std::filesystem::path imageFile)
{
	FreeData();

	std::ifstream file(imageFile, std::ios::binary);
	if(!file.is_open())
	{
		std::cerr<< "Couldn't open image " << imageFile.string()<<"\n";
		return false;
	}

	struct{
		uint32_t type;
		uint32_t size;
		uint32_t cSize;
		uint16_t w,h;
	} meta{};
	file.read((char*)&meta, sizeof(meta));
	if((meta.type&~0x1000) > 4 || meta.w > 16384 || meta.h > 16384 || meta.size*2 < meta.cSize) //Checking valid header.
		return false;

	*alloc.ptr = alloc.allocate(meta.size);
	auto cData = std::make_unique<char[]>(meta.cSize);
	file.read(cData.get(), meta.cSize);

	int decompSize = LZ4_decompress_safe(cData.get(), (char*)*alloc.ptr, meta.cSize, meta.size);
	if(decompSize < 0 || decompSize != meta.size)
	{
		alloc.deallocate(alloc.ptr);
		std::cerr << "Texture::LoadLzs3: Error while decompressing LZ4 - " << imageFile.filename()<< "\n";
		return false;
	}

	constexpr uint32_t nonS3tcFlag = 0x1000;
	bytesPerPixel = meta.size/(meta.w*meta.h);
	compressed = !(meta.type & nonS3tcFlag);

	width = meta.w;
	height = meta.h;

	return true;
}	

bool ImageData::WriteRaw(std::filesystem::path filename) const
{
	uint32_t size = GetMemSize();
	std::ofstream out(filename, std::ios_base::binary);
	if(!out.is_open())
		return false;
	
	raw_header meta = {
		width, height, bytesPerPixel
	};
	
	out.write((char*)&meta, sizeof(meta));
	out.write((char*)data, size);
	return true;
}