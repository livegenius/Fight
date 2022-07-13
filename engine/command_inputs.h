#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include <vector>
#include <deque>
#include <string>
#include <map>
#include <filesystem>
#include <unordered_map>
#include <sol/sol.hpp>

struct InputBuffer{
	size_t lastLoc = 0;
	std::vector<uint32_t> buffer;

	uint32_t back() const{
		return buffer[lastLoc];
	}

	uint32_t operator[](size_t index) const{
		return buffer[index];
	}

	size_t size() const{return lastLoc+1;}
}; //"Buffer" containing processed input.

struct MotionData
{
	std::string motionStr; //Without the button press.
	sol::protected_function condition; //Without the button press.
	bool hasCondition = false;
	int startBuf = 0;
	int addBuf = 0;
	int seqRef = -1;
	int flags = 0;
	int priority = 0x7FFFFFFF; //Less is higher priority
};

struct ChargeState
{
	struct Charges{
		uint16_t dirCharge[4];
	} charges;
	std::deque<Charges> chargeBuffer;

	ChargeState();
	void Charge(uint32_t keyPress);

	enum dir{
		up,
		down,
		left,
		right
	};
	int GetCharge(dir which, int frame, int side = 1 ) const;
};

class CommandInputs
{
	std::unordered_map<std::string, std::vector<MotionData>> motions;

public:
	struct CancelInfo
	{
		int subFrameCount;
		uint32_t actorFlags;
		bool canNormalCancel;
		bool canSpecialCancel;
		bool canInterrupt;
		int currentSequence;
	};

	CommandInputs() = default;

	void LoadFromLua(std::filesystem::path defFile, sol::state &lua);

	//Returns sequence number and flags.
	MotionData ProcessInput(const InputBuffer &keyPresses, const ChargeState &charge, const char*motionType, int side, CancelInfo info);
	

private:
	bool MotionInput(const MotionData& md, const InputBuffer &keyPresses, const ChargeState &charge, int side);



	
public:
	enum //flags
	{
		neutralMove = 0x1,
		repeatable = 0x2,
		wipeBuffer = 0x4,
		interrupts = 0x8, //Will interrupt any move marked as interruptible.
		interruptible = 0x10,
		noCombo = 0x20
	};

};

int SanitizeKey(int lever);

#endif // INPUT_H_INCLUDED
