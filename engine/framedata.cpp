#include "framedata.h"
#include <iostream>
#include <fstream>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

glm::mat4 CalculateTransform(float offset[2], float rotation[3], float scale[2])
{
	float rotX = rotation[0];
	float rotY = rotation[1];
	float rotZ = rotation[2];
	float offsetX = offset[0];
	float offsetY = offset[1];
	float scaleX = scale[0];
	float scaleY = scale[1];
	
	constexpr float tau = glm::pi<float>()*2.f;
	glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(scaleX, scaleY, 1.f));
	transform = glm::rotate(transform, -rotZ*tau, glm::vec3(0.0, 0.f, 1.f));
	transform = glm::rotate(transform, rotY*tau, glm::vec3(0.0, 1.f, 0.f));
	transform = glm::rotate(transform, rotX*tau, glm::vec3(1.0, 0.f, 0.f));
	return transform;
}

bool LoadSequences(std::vector<Sequence> &sequences, std::filesystem::path charFile, sol::state &lua)
{
	constexpr const char *charSignature = "AFGECharacterFile";
	constexpr uint32_t currentVersion = 99'6;

	io::Framedata fd;

	if(!fd.LoadFD(charFile))
	{
		std::cerr << "Couldn't load character file.\n";
		return false;
	}

	auto seqN = fd.sequences.size();
	sequences.resize(seqN);
	for(size_t seqI = 0; seqI < seqN; ++seqI)
	{
		auto &seq = sequences[seqI];
		auto &inputSeq = fd.sequences[seqI];
		seq.props = std::move(inputSeq.props);

		if(!inputSeq.function.empty())
		{
			seq.function = lua[inputSeq.function];
			if(seq.function.get_type() == sol::type::function)
				seq.hasFunction = true;
			else
				std::cerr << "Unknown function "<<inputSeq.function<<" in sequence "<<seqI<<"\n";
		}
		
		auto frameN = inputSeq.frames.size();
		seq.frames.resize(frameN);
		for (size_t frameI = 0; frameI < frameN; ++frameI)
		{
			auto &frame = seq.frames[frameI];
			auto &inputFrame = inputSeq.frames[frameI];
			frame.frameProp = std::move(inputFrame.frameProp);

			//Precalculate matrix transformation
			auto &fp = frame.frameProp;
			frame.transform = CalculateTransform(fp.spriteOffset, fp.rotation, fp.scale);
			
			auto boxN = inputFrame.greenboxes.size();
			frame.greenboxes.resize(boxN/4);
			for(int bi = 0; bi < boxN; bi+=4)
			{
				frame.greenboxes[bi/4] = Rect2d<FixedPoint>(
					inputFrame.greenboxes[bi+0],inputFrame.greenboxes[bi+1],
					inputFrame.greenboxes[bi+2],inputFrame.greenboxes[bi+3]);
			}
			boxN = inputFrame.redboxes.size();
			frame.redboxes.resize(boxN/4);
			for(int bi = 0; bi < boxN; bi+=4)
			{
				frame.redboxes[bi/4] = Rect2d<FixedPoint>(
					inputFrame.redboxes[bi+0],inputFrame.redboxes[bi+1],
					inputFrame.redboxes[bi+2],inputFrame.redboxes[bi+3]);
			}

			if(inputFrame.colbox.size()>=4)
			{
				frame.colbox.bottomLeft.x = inputFrame.colbox[0];
				frame.colbox.bottomLeft.y = inputFrame.colbox[1];
				frame.colbox.topRight.x   = inputFrame.colbox[2];
				frame.colbox.topRight.y   = inputFrame.colbox[3];
			}
		}
	}

	return true;
}