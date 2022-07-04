#ifndef FRAMEDATA_IO_H_GUARD
#define FRAMEDATA_IO_H_GUARD

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace io{

struct FrameProperty
{
	int32_t spriteIndex = 0;
	int32_t duration = 0;
	int32_t jumpTo = 0;
	int32_t jumpType = 0;
	bool relativeJump = false;
	uint32_t flags = 0;
	int32_t vel[2] = {0}; // x,y
	int32_t accel[2] = {0};
	int32_t movementType[2] = {0}; //Add or set X,Y
	int16_t cancelType[2] = {};
	int32_t state = 0;
	float spriteOffset[2]; //x,y
	int16_t loopN;
	int16_t chType;
	float scale[2];
	float color[4];
	int32_t blendType = 0;
	float rotation[3]; //XYZ
};

struct Frame
{
	FrameProperty frameProp;
	std::string frameScript;
	//Attack_property attackProp;
	//Boxes are defined by BL, BR, TR, TL points, in that order.
	std::vector<int> greenboxes;
	std::vector<int> redboxes;
	std::vector<int> colbox;
};

struct SequenceProperty
{
	int level = 0;
	int landFrame = 0;
	int zOrder = 0;
	uint32_t flags = 0;
};

struct Sequence
{
	SequenceProperty props;
	std::vector<Frame> frames;
	std::string name;
	std::string function;
};

class Framedata
{
public:
	std::vector<Sequence> sequences;

public:
	bool LoadFD(std::filesystem::path file);
	void SaveFD(std::filesystem::path file);

	bool LoadChar(std::string charFile);
	void Clear();
	void Close();
	void SaveChar(std::string charFile);
	
	std::pair<std::string, int> GetDecoratedName(int n);
	bool loaded = false;
};

}
#endif /* FRAMEDATA_IO_H_GUARD */
