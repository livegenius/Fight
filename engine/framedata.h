#ifndef FRAMEDATA_H_GUARD
#define FRAMEDATA_H_GUARD

#include <fixed_point.h>
#include <geometry.h>
#include <string>
#include <vector>
#include <filesystem>
#include <sol/sol.hpp>
#include <glm/mat4x4.hpp>
#include <framedata_io.h>

constexpr int speedMultiplier = 240;

struct Frame
{
	io::FrameProperty frameProp;
	//Attack_property attackProp;
	//Boxes are defined by BL, BR, TR, TL points, in that order.
	typedef std::vector<Rect2d<FixedPoint>> boxes_t;
	boxes_t greenboxes;
	boxes_t redboxes;
	Rect2d<FixedPoint> colbox;
	glm::mat4 transform;
};

struct Sequence
{
	io::SequenceProperty props;
	std::vector<Frame> frames;
	//std::string name;
	sol::protected_function function;
	bool hasFunction = false;
};

bool LoadSequences(std::vector<Sequence> &sequences, std::filesystem::path charfile, sol::state &lua);

namespace flag{
	enum //frame-dependent bit-mask flags
	{
		canMove = 0x1,
		dontWalk = 0x2,
		
		startHit = 0x8000'0000
	};
}

namespace jump{
	enum
	{
		none = 0,
		frame,
		loop,
		seq
	};
}

namespace state{
	enum
	{
		stand,
		crouch,
		air,
		otg,
	};
}

#endif /* FRAMEDATA_H_GUARD */

