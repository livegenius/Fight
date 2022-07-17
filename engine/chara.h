#ifndef CHARACTER_H_INCLUDED
#define CHARACTER_H_INCLUDED

#include "battle_interface.h"
#include "framedata.h"
#include "camera.h"
#include "command_inputs.h"
#include "fixed_point.h"
#include "actor.h"
#include <geometry.h>

#include <deque>
#include <string>
#include <vector>

#include <sol/sol.hpp>

class Character : public Actor
{
private:
	friend class Player;
	Character *target = nullptr;
	Actor *wallPushbackTarget = nullptr;
	//sol::state lua;

	int health = 10000;
	//int hitsTaken;
	//inr damageTaken;
	int hurtSeq = -1;
	bool gotHit = false;

	bool isAlreadyBlocking = false;
	int hitFlags = 0; //When getting hit

	HitDef::Vector bounceVector;
	int blockTime = 0;
	int pushTimer = 0; //Counts down the pushback time.

	BattleInterface* scene;
	
	bool interruptible = false;
	bool mustTurnAround = false;
	bool successfulInput = false; //Set to true if action was performed successfully due to player input
	bool whiffed = true;

	//FixedPoint getAway; //Amount to move after collision
	FixedPoint touchedWall; //left wall: -1, right wall = 1, no wall = 0;
	MotionData lastCommand;




public:
	Character(FixedPoint posX, int side, BattleInterface& scene, sol::state &lua, std::vector<Sequence> &sequences, std::vector<Actor> &actorList);
	bool Update();
	
	void BoundaryCollision(); //Collision against stage
	void Input(InputBuffer &keyPresses, const ChargeState &charges, CommandInputs &cmd);

private:
	
	void SetPos(FixedPoint x, FixedPoint y);
	void Translate(Point2d<FixedPoint> amount);
	void Translate(FixedPoint x, FixedPoint y);
	void GotoSequence(int seq);
	void GotoSequenceMayTurn(int seq);
	int ResolveHit(int keypress, Actor *hitter, bool AlwaysBlock = false);
	bool TurnAround(int sequence = -1);
};


struct PlayerStateCopy
{
	sol::table luaState;
	std::unique_ptr<Character> charObj;
	std::vector<Actor> children;
	Character* target;
	ChargeState chargeState;
	unsigned int lastKey[2]{};
	int priority;
};

class Player
{
private:
	sol::state lua;
	sol::protected_function updateFunction;
	sol::protected_function aiFunction;
	bool hasUpdateFunction = false;

	std::vector<Sequence> sequences;
	std::vector<Actor> newChildren;
	BattleInterface& scene;
	Character* charObj = nullptr;
	Character* target = nullptr;
	Player* pTarget = nullptr;
	ChargeState chargeState;

	unsigned int lastKey[2]{};
	CommandInputs cmd;

	bool ScriptSetup(bool ai);
	bool aiPlayer;

public:
	int priority = 0;
	std::vector<Actor> children;
	struct DrawList{
		std::vector<Actor*> v;
		int middle;
		void Init(const Player &p1, const Player &p2)
		{
			v.resize(p1.children.size()+p2.children.size()+2);
			middle = 0;
		}
	};
	
	Player(BattleInterface& scene);
	~Player();
	Player(int side, std::string charFile, BattleInterface& scene, int paletteSlot, bool ai = false);
	void Load(int side, std::string charFile, int paletteSlot, bool ai = false);
	void SetState(PlayerStateCopy &state);
	PlayerStateCopy GetStateCopy();

	void SetTarget(Player &target);
	void Update(HitboxRenderer *hr);
	int FillDrawList(DrawList &dl); //Returns player object index in the drawlist
	void ProcessInput(InputBuffer inputs);
	Point2d<FixedPoint> GetXYCoords();
	float GetHealthRatio();

	static void HitCollision(Player &blue, Player &red); //Checks hit/hurt box collision and sets flags accordingly.
	static void Collision(Player &blue, Player &red); //Detects and resolves collision between characters and/or the camera.
};



#endif // CHARACTER_H_INCLUDED
