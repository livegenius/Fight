#ifndef ACTOR_H_GUARD
#define ACTOR_H_GUARD

#include "hitbox_renderer.h"
#include "framedata.h"
#include <geometry.h>
#include <fixed_point.h>
#include <unordered_map>
#include <sol/sol.hpp>
#include <glm/mat4x4.hpp>

const FixedPoint floorPos(32);

struct HitDef
{
	struct Vector
	{
		int maxPushBackTime;
		int xSpeed, ySpeed;
		int xAccel, yAccel;
		std::string sequenceName;
		std::string bounceTable;
	};
	//key is type (air, cro, sta), array value is vector subtable (hit and block)
	std::unordered_map<int, std::array<Vector, 2>> vectorTables;
	int attackFlags = 0;
	int damage = 0;
	int guardDamage = 0;
	int correction = 0;
	int correctionType = 0;
	int meterGain = 0;
	int hitStop = 0;
	int blockStop = -1;
	int untech = 0;
	int blockstun = 0; //Untech and block
	int priority = 0;
	int soundFx = 0;
	int hitFx = 0;
	int shakeTime;

	void Clear();
	void SetVectors(int state, sol::table onHit, sol::table onBlock);

	enum flag{
		canBounce = 0x1,
		hitsCrouch = 0x2,
		hitsAir = 0x4,
		hitsStand = 0x8,
		unblockable = hitsCrouch | hitsAir | hitsStand,
		wallBounce = 0x10,
		disableCollision = 0x20,
	};

	static Vector getVectorTableFromTable(const sol::table &table);
};

struct RenderOptions
{
	enum
	{
		normal,
		additive
	};
	int blendingMode;
	int paletteIndex;
	float rgba[4];
};

class Actor{
	friend class Character;
	friend class Player;
	std::vector<Sequence> *sequences;

protected:
	std::reference_wrapper<std::vector<Actor>> actorList;
	std::reference_wrapper<sol::state> lua;

	Actor* attachPoint = nullptr;
	HitDef attack;
	//sol::state &lua;

	Point2d<FixedPoint> root; //Character (x,y) position in game. Every box position is relative to this.
	Point2d<FixedPoint> pastRoot;
	Point2d<FixedPoint> vel;
	Point2d<FixedPoint> accel;
	Sequence *seqPointer;
	Frame *framePointer;
	
	int paletteIndex; //Palette slot to use for indexed sprites
	int side = 1; //used to invert the x of all sort of things

	int currSeq = 0; //The active sequence.
	int currFrame = 0;
	int landingFrame = 0;
	int frameDuration; //counter for changing frames
	int loopCounter = 0;
	int totalSubframeCount = 0;
	int	subframeCount = 0;
	int hitstop = 0; //hitstop counter

	int hitCount = 0;
	bool friction = false;
	bool frozen = false;
	bool hittable = false;
	bool shaking = false; //Shakes on hitstop.
	enum hitType {
		none,
		hurt,
		blocked,
		counter,
	};
	//comboType is set to hurt/blocked by the target. Resets when sequence changes. Used for cancelling purposes.
	int comboType = none; 
	uint32_t flags = 0;
	std::unordered_map<std::string, sol::object> userData;
	glm::mat4 customTransform = glm::mat4(1);

public:
	Actor(std::vector<Sequence> &sequences, sol::state &lua, std::vector<Actor> &actorList);
	~Actor();

	virtual bool Update();
	bool AdvanceFrame();
	virtual void GotoSequence(int seq);
	bool GotoFrame(int frame);
	void Translate(Point2d<FixedPoint> amount);
	void Translate(FixedPoint x, FixedPoint y);
	void SetSide(int side);
	int GetSide();

	Actor& SpawnChild(int sequence = 0);

	Frame *GetCurrentFrame();
	int GetSpriteIndex();
	glm::mat4 GetSpriteTransform();
	RenderOptions GetRenderOptions();

	//If collided and where
	static std::pair<bool, Point2d<FixedPoint>> HitCollision(const Actor& hurt, const Actor& hit);
	static void DeclareActorLua(sol::state &lua);

	void SendHitboxData(HitboxRenderer &hr);

protected:
	void SeqFun();
	void SetHitDef(sol::table onHit, sol::table onBlock);
	virtual int ResolveHit(int keypress, Actor *hitter);

	bool ThrowCheck(Actor& enemy, int frontRange, int upRange, int downRange);
	int SetVectorFromTable(const sol::table &table, int side);

	enum actorFlags{
		floorCheck = 0x1,
		followParent = 0x2,
		noCollision = 0x4,
	};
};

#endif /* ACTOR_H_GUARD */
