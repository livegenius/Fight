#include "framedata_io.h"
//#include "render.h"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <bitsery/bitsery.h>
#include <bitsery/adapter/stream.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
#include <bitsery/ext/compact_value.h>

#include <lz4.h>

namespace io{

template <typename S>
void serialize(S &s, FrameProperty &o)
{
	s.value4b(o.spriteIndex);
	s.value4b(o.duration);
	s.value4b(o.jumpTo);
	s.value4b(o.jumpType);
	s.value4b(o.flags);
	s.value4b(o.state);
	s.value4b(o.blendType);
	s.value2b(o.loopN);
	s.value2b(o.chType);
	s.container4b(o.spriteOffset);
	s.container4b(o.rotation);
	s.container4b(o.scale);
	s.container4b(o.color);
	s.container4b(o.vel);
	s.container4b(o.accel);
	s.container4b(o.movementType);
	s.container2b(o.cancelType);
	s.value1b(o.relativeJump);
}

template <typename S>
void serialize(S &s, Frame &o){
	constexpr size_t maxBoxes = 4*256;
	s.object(o.frameProp);
	//s.text1b(o.frameScript, 128);
	s.container4b(o.greenboxes, maxBoxes);
	s.container4b(o.redboxes, maxBoxes);
	s.container4b(o.colbox, maxBoxes);
}

template <typename S>
void serialize(S &s, SequenceProperty&o)
{
	s.value4b(o.level);
	s.value4b(o.landFrame);
	s.value4b(o.zOrder);
	s.value4b(o.flags);
}

template <typename S>
void serialize(S &s, Sequence &o)
{
	s.object(o.props);
	s.container(o.frames, 1024);
	s.text1b(o.name, 128);
	s.text1b(o.function, 128);
}

struct Header{
	char signature[16];
	uint32_t version;
	uint32_t fileSizeCompressed;
	uint32_t fileSize;

	template <typename S>
	void serialize(S &s){
		s.text1b(signature);
		s.value4b(version);
		s.value4b(fileSize);
		s.value4b(fileSizeCompressed);
	}
};

constexpr Header CurrentHeader {"Fight Framedata", 'v001', 0, 0};

struct Context{
	int version = CurrentHeader.version;
} context;

bool Framedata::LoadFD(std::filesystem::path path)
{
	using namespace bitsery;
	std::ifstream file(path, std::ifstream::in | std::ifstream::binary);

	if (!file.is_open())
		return false;

	//Read header
	Header header;
	Deserializer<InputStreamAdapter> desFile{file};
	auto &adapter = desFile.adapter();
	desFile.object(header);
	
	if(adapter.error() == ReaderError::NoError 
		&& strncmp(header.signature, CurrentHeader.signature, sizeof(Header::signature)) != 0)
		return false;

	//Decompress
	std::vector<char> compressedBuf(header.fileSizeCompressed);
	desFile.container1b(compressedBuf, 0x4000000);

	std::vector<char> data(header.fileSize);
	auto bufSize = LZ4_decompress_safe(compressedBuf.data(), data.data(), header.fileSizeCompressed, header.fileSize);
	if(bufSize < 0 || bufSize != header.fileSize)
		return false;

	//Deserialize decompressed data
	InputBufferAdapter<decltype(data)> inputAdapter{data.begin(), header.fileSize};
	Deserializer<decltype(inputAdapter), Context> desBuf{context, std::move(inputAdapter)};
	desBuf.container(sequences, 0xFFFF);

	bool success = adapter.error() == ReaderError::NoError && adapter.isCompletedSuccessfully();
	loaded = success;
	return success;
}

void Framedata::SaveFD(std::filesystem::path path)
{
	using namespace bitsery;
	std::ofstream file(path, std::ofstream::out | std::ifstream::binary);


	std::vector<char> data;
	Serializer<OutputBufferAdapter<decltype(data)>, Context> serBuf{context, data};
	serBuf.container(sequences, 0xFFFF);
	serBuf.adapter().flush();

	//Compress
	auto bufSize = serBuf.adapter().writtenBytesCount();
	auto maxCSize = LZ4_compressBound(bufSize);
	std::vector<char> compressedBuf(maxCSize);
	auto cSize = LZ4_compress_default(data.data(), compressedBuf.data(), bufSize, maxCSize);
	compressedBuf.resize(cSize);

	Header header = CurrentHeader;
	header.fileSizeCompressed = cSize;
	header.fileSize = bufSize;

	Serializer<OutputBufferedStreamAdapter> serFile{file};
	serFile.object(header);
	serFile.container1b(compressedBuf, cSize);
	serFile.adapter().flush();
	return;
}


struct BoxSizes
{
	int8_t greens;
	int8_t reds;
	int8_t collision;
};

std::pair<std::string, int> Framedata::GetDecoratedName(int n)
{
	std::stringstream ss;
	ss.flags(std::ios_base::right);
	
	ss << std::setfill('0') << std::setw(3) << n << " - " << sequences[n].name;

	bool hasScript = false;
	for(const auto& f : sequences[n].frames)
		if(!f.frameScript.empty())
		{
			hasScript = true;
			break;
		}
	return {ss.str(), hasScript};
}

void Framedata::Clear()
{
	sequences.clear();
	sequences.resize(1000);
	loaded = true;
}

void Framedata::Close()
{
	sequences.clear();
	loaded = false;
}

//Legacy serialization.

#define rv(X) ((char*)&X)
#define rptr(X) ((char*)X)

constexpr const char *charSignature = "AFGECharacterFile";
constexpr uint32_t currentVersion = 99'6;

struct CharFileHeader //header for .char files.
{
	char signature[32];
	uint32_t version;
	uint16_t sequences_n;
};

bool Framedata::LoadChar(std::string charFile)
{
	//loads character from a file and fills sequences/frames and all that yadda.
	std::ifstream file(charFile, std::ios_base::in | std::ios_base::binary);
	if (!file.is_open())
	{
		std::cerr << "Couldn't open character file.\n";
		return false;
	}

	CharFileHeader header;
	file.read(rv(header), sizeof(CharFileHeader));
	if(strcmp(charSignature, header.signature))
	{
		std::cerr << "Signature mismatch.\n";
		return false;
	}
	if(header.version != currentVersion)
	{
		std::cerr << "Format version mismatch.\n";
		return false;
	}

	sequences.resize(header.sequences_n);
	for (uint16_t i = 0; i < header.sequences_n; ++i)
	{
		auto &currSeq = sequences[i];
		uint8_t size;
		file.read(rv(size), sizeof(size));
		currSeq.name.resize(size);
		file.read(rptr(currSeq.name.data()), size);

		file.read(rv(size), sizeof(size));
		currSeq.function.resize(size);
		file.read(rptr(currSeq.function.data()), size);

		file.read(rv(currSeq.props), sizeof(SequenceProperty));

		uint8_t seqlength;
		file.read(rv(seqlength), sizeof(seqlength));
		currSeq.frames.resize(seqlength);
		for (uint8_t i2 = 0; i2 < seqlength; ++i2)
		{
			auto &currFrame = currSeq.frames[i2];

			file.read(rv(size), sizeof(size));
			currFrame.frameScript.resize(size);
			file.read(rptr(currFrame.frameScript.data()), size);

			//How many boxes are used per frame
			BoxSizes bs;
			file.read(rv(bs), sizeof(BoxSizes));
			currFrame.greenboxes.resize(bs.greens);
			currFrame.redboxes.resize(bs.reds);
			currFrame.colbox.resize(bs.collision);

			file.read(rv(currFrame.frameProp), sizeof(FrameProperty));

			file.read(rptr(currFrame.greenboxes.data()), sizeof(int) * bs.greens);
			file.read(rptr(currFrame.redboxes.data()), sizeof(int) * bs.reds);
			file.read(rptr(currFrame.colbox.data()), sizeof(int) * bs.collision);
		}
	}

	file.close();
	loaded = true;
	return true;
}

void Framedata::SaveChar(std::string charFile)
{
	//loads character from a file and fills sequences/frames and all that yadda.
	std::ofstream file(charFile, std::ios_base::out | std::ios_base::binary);
	if (!file.is_open())
	{
		std::cerr << "Couldn't open character file.\n";
		return;
	}

	CharFileHeader header;
	header.sequences_n = sequences.size();
	header.version = currentVersion;
	strncpy_s(header.signature, charSignature, 32);
	file.write(rv(header), sizeof(CharFileHeader));

	for (uint16_t i = 0; i < header.sequences_n; ++i)
	{
		auto &currSeq = sequences[i];
		uint8_t strSize = currSeq.name.size();
		file.write(rv(strSize), sizeof(strSize));
		file.write(rptr(currSeq.name.data()), strSize);

		strSize = currSeq.function.size();
		file.write(rv(strSize), sizeof(strSize));
		file.write(rptr(currSeq.function.data()), strSize);

		file.write(rv(currSeq.props), sizeof(SequenceProperty));

		uint8_t seqlength = currSeq.frames.size();
		file.write(rv(seqlength), sizeof(seqlength));
		for (uint8_t i2 = 0; i2 < seqlength; ++i2)
		{
			auto &currFrame = currSeq.frames[i2];

			strSize = currFrame.frameScript.size();
			file.write(rv(strSize), sizeof(strSize));
			file.write(rptr(currFrame.frameScript.data()), strSize);

			//How many boxes are used per frame
			BoxSizes bs;
			bs.greens = currFrame.greenboxes.size();
			bs.reds = currFrame.redboxes.size();
			bs.collision = currFrame.colbox.size();
			file.write(rv(bs), sizeof(BoxSizes));

			file.write(rv(currFrame.frameProp), sizeof(FrameProperty));

			file.write(rptr(currFrame.greenboxes.data()), sizeof(int) * bs.greens);
			file.write(rptr(currFrame.redboxes.data()), sizeof(int) * bs.reds);
			file.write(rptr(currFrame.colbox.data()), sizeof(int) * bs.collision);
		}
	}

	file.close();
	return;
}

#undef rv
#undef rptr

}