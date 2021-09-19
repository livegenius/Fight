#include "image.h"
#include <spng.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

ImageData::ImageData():
data(nullptr), width(0), height(0), bytesPerPixel(0)
{}

ImageData::ImageData(uint32_t _width, uint32_t _height, uint32_t _bytesPerPixel):
data(nullptr), width(_width), height(_height), bytesPerPixel(_bytesPerPixel)
{
	data = (uint8_t*) malloc(width * height * bytesPerPixel * sizeof(uint8_t));
}

ImageData::ImageData(std::filesystem::path image): ImageData()
{
	if(!LoadFromPng(image) && !LoadRaw(image))
		std::cerr << image<<": invalid image file\n";
}

ImageData::~ImageData()
{
	free(data);
}

std::size_t ImageData::GetMemSize() const
{
	return width*height*bytesPerPixel;
}

void ImageData::FreeData()
{
	free(data);
	data = nullptr;
}

struct raw_header{
	uint32_t w, h, bpp;
};

bool ImageData::WriteRaw(std::filesystem::path filename) const
{
	std::ofstream out(filename, std::ios_base::binary);
	if(!out.is_open())
		return false;
	
	raw_header meta = {
		width, height, bytesPerPixel
	};
	uint32_t size = GetMemSize();
	out.write((char*)&meta, sizeof(meta));
	out.write((char*)data, size);
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
	data = (uint8_t*)malloc(size);
	in.read((char*)data, size);
	return true;
}

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

bool ImageData::LoadFromPng(std::filesystem::path filename)
{
	FreeData();
	std::ifstream png(filename, std::ios_base::binary);
	int r;

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

	std::unique_ptr<uint8_t> out((unsigned char*)malloc(out_size));
	if(out == nullptr) return false;

	/* This is required to initialize for progressive decoding */
	r = spng_decode_image(ctx, NULL, 0, fmt, SPNG_DECODE_PROGRESSIVE);
	if(r)
	{
		printf("progressive spng_decode_image() error: %s\n", spng_strerror(r));
		return false;
	}

	/* ihdr.height will always be non-zero if spng_get_ihdr() succeeds */
	out_width = out_size / ihdr.height;
	assert((out_size / ihdr.height)/ihdr.width == bytesPerPixel);

	struct spng_row_info row_info = {0};

	do
	{
		r = spng_get_row_info(ctx, &row_info);
		if(r) break;

		//Decode upside down as usual.
		r = spng_decode_row(ctx, out.get()+out_size - out_width - row_info.row_num * out_width, out_width);
	}
	while(!r);

	if(r != SPNG_EOI)
	{
		printf("progressive decode error: %s\n", spng_strerror(r));

		if(ihdr.interlace_method)
			printf("last pass: %d, scanline: %" PRIu32 "\n", row_info.pass, row_info.scanline_idx);
		else
			printf("last row: %" PRIu32 "\n", row_info.row_num);
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
	data = out.release();

	return true;
}

