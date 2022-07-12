#include <cmath>
#include <deque>
#include <fstream>
#include <string>
#include <iostream>
#include <limits>

#include "chara.h"
#include "raw_input.h" //Used only by Character::ResolveHit

Character::Character(FixedPoint xPos, int side, BattleInterface& scene, sol::state &lua, std::vector<Sequence> &sequences, std::vector<Actor> &actorList) :
Actor(sequences, lua, actorList),
touchedWall(0),
scene(&scene)
{
	root.x = xPos;
	root.y = floorPos;
	SetSide(side);
	hittable = true;
	//userData = lua.create_table();
	return;
}

void Character::BoundaryCollision()
{
	Camera &currView = scene->view;
	const FixedPoint wallOffset(10);
	touchedWall = 0;

	if (root.x <= currView.GetWallPos(camera::leftWall) + wallOffset)
	{
		touchedWall = -1;
		root.x = currView.GetWallPos(camera::leftWall) + wallOffset;
	}

	else if (root.x >= currView.GetWallPos(camera::rightWall) - wallOffset)
	{
		touchedWall = 1;
		root.x = currView.GetWallPos(camera::rightWall) - wallOffset;
	}
	
	if (touchedWall != 0 && hitFlags & HitDef::wallBounce)
	{
		hitFlags &= ~HitDef::wallBounce;
		vel.x.value = bounceVector.xSpeed*speedMultiplier;
		vel.y.value = bounceVector.ySpeed*speedMultiplier;
		accel.x.value = bounceVector.xAccel*speedMultiplier;
		accel.y.value = bounceVector.yAccel*speedMultiplier;
		pushTimer = bounceVector.maxPushBackTime;
		GotoSequence(lua.get()["_seqTable"][bounceVector.sequenceName].get_or(-1));
		touchedWall = 0;
		hitstop = 6;
		scene->view.SetShakeTime(12);
		scene->sfx.PlaySound("wallBounce");
	}
	else if (touchedWall == target->touchedWall) //Someone already has the wall.
		touchedWall = 0;
}


int Character::ResolveHit(int keypress, Actor *hitter, bool AlwaysBlock)
{
	int retType = hitType::hurt;
	HitDef *hitData = &hitter->attack;
	keypress = SanitizeKey(keypress);
	
	int left;
	int right;
	if (GetSide() < 0) //Inverts absolute input depending on side. Apparent input is corrected.
	{
		left = key::buf::RIGHT;
		right = key::buf::LEFT;
	}
	else
	{
		left = key::buf::LEFT;
		right = key::buf::RIGHT;
	}

	bool blocked = false;
	int state = framePointer->frameProp.state;

	bool triesToBlock = keypress & left;
	if(AlwaysBlock)
	{
		triesToBlock = true;
		//Crouch block if necessary or already crouch blocking, unless the attack hits crouchers.
		if(hitData->attackFlags & HitDef::hitsStand ||
			(framePointer->frameProp.state == state::crouch && !(hitData->attackFlags & HitDef::hitsCrouch)))
			keypress |= key::buf::DOWN;
	}

	//Can block
	if ((isAlreadyBlocking) || (framePointer->frameProp.flags & flag::canMove && triesToBlock))
	{
		const auto &st = framePointer->frameProp.state;
		const auto &flag = hitData->attackFlags;
		bool groundedState = st == state::crouch || st == state::stand;
		//Blocked successfully
		if(!(
			(flag & HitDef::hitsAir && st == state::air) ||
			(flag & HitDef::hitsCrouch && groundedState && keypress & key::buf::DOWN) ||
			(flag & HitDef::hitsStand && groundedState && !(keypress & key::buf::DOWN))
		))
		{
			if(hitData->blockStop>=0)
				hitstop = hitData->blockStop;
			else
				hitstop = hitData->hitStop;
			blocked	= true;
			isAlreadyBlocking = true;
			blockTime = hitData->blockstun;
			scene->sfx.PlaySound("block");
			retType = hitType::blocked;
			if(groundedState)
				state = keypress & key::buf::DOWN ? state::crouch : state::stand;
		}
	}

	if(hitData->vectorTables.count(state) == 0)
		state = 0; //TODO: Fallback state from lua?
	if(hitData->vectorTables.count(state) > 0)
	{
		auto &vt = hitData->vectorTables[state][blocked];
		int seq = lua.get()["_seqTable"][vt.sequenceName].get_or(-1);
		if(seq > 0)
		{
			vel.x.value = vt.xSpeed*speedMultiplier*hitter->side;
			vel.y.value = vt.ySpeed*speedMultiplier;
			accel.x.value = vt.xAccel*speedMultiplier*hitter->side;
			accel.y.value = vt.yAccel*speedMultiplier;
			pushTimer = vt.maxPushBackTime;	
			friction = true;

			if(!blocked) // No wall/floor bounce or other weird stuff. Should be a separate flag maybe.
			{
				hitFlags = hitData->attackFlags;
				sol::optional<sol::table> t = lua.get()["_vectors"][vt.bounceTable];
				if(t)
				{
					bounceVector = hitData->getVectorTableFromTable(t.value());
					bounceVector.xSpeed *= hitter->side;
					bounceVector.xAccel *= hitter->side;
				}
			}		

			gotHit = true;
			hurtSeq = seq;
			shaking = true;
			//TODO: Call lua hit func Here.
		}
	}

	if(!blocked)
	{
		isAlreadyBlocking = false;
		hitstop = hitData->hitStop;
		scene->view.SetShakeTime(hitData->shakeTime);
		health -= hitData->damage;
		if(!hitData->hitSound.empty())
			scene->sfx.PlaySound(hitData->hitSound);
		if(framePointer->frameProp.chType > 0)
		{
			hitstop = hitstop*2 + 2 + 5*(framePointer->frameProp.state == state::air);
			scene->sfx.PlaySound("counter");
			return hitType::counter;
		}
	}
	return retType;
}

void Character::Translate(Point2d<FixedPoint> amount)
{
	root += amount;
	BoundaryCollision();
}

void Character::Translate(FixedPoint x, FixedPoint y)
{
	root.x += x;
	root.y += y;
	BoundaryCollision();
}

void Character::GotoSequence(int seq)
{
	if (seq < 0)
		return;
	
	interruptible = false;
	comboType = none;
	currSeq = seq;
	currFrame = 0;
	totalSubframeCount = 0;
	attack.Clear();
	hitCount = 0;

	seqPointer = &(*sequences)[currSeq];
	landingFrame = seqPointer->props.landFrame;
	flags &= ~noCollision; 
	GotoFrame(0);
}

void Character::GotoSequenceMayTurn(int seq)
{
	if(mustTurnAround)
	{
		mustTurnAround = false;
		side = -side;
	}

	GotoSequence(seq);
}

bool Character::Update()
{
	pastRoot = root;
	if(attachPoint)
	{
		root += attachPoint->root - attachPoint->pastRoot;
	}
	if(frozen)
		return true;
	if(gotHit) //Chara
	{
		GotoSequence(hurtSeq);
		if(hitFlags & HitDef::disableCollision)
			flags |= noCollision;
		hurtSeq = -1;
		gotHit = false;
		if (root.y < floorPos) //Fixes a problem when you get hit under the floor.
			root.y = floorPos;
	}
	if (hitstop > 0)
	{
		//Shake effect
		--hitstop;
		return true;
	}
	else
		shaking = false;

	int advanced = AdvanceFrame();
	if(advanced == -1) //Died
		return false;
	else if(advanced == 1 && framePointer->frameProp.flags & flag::canMove)
	{
		friction = false;
		isAlreadyBlocking = false;
		pushTimer = 0;
	}

	if (root.y < floorPos && !hitstop) //Check collision with floor
	{
		root.y = floorPos;
		if(hitFlags & HitDef::canBounce)
		{
			hitFlags &= ~HitDef::canBounce;
			vel.x.value = bounceVector.xSpeed*speedMultiplier;
			vel.y.value = bounceVector.ySpeed*speedMultiplier;
			accel.x.value = bounceVector.xAccel*speedMultiplier;
			accel.y.value = bounceVector.yAccel*speedMultiplier;
			pushTimer = bounceVector.maxPushBackTime;
			GotoSequence(lua.get()["_seqTable"][bounceVector.sequenceName].get_or(-1));
			scene->view.SetShakeTime(12);
			scene->sfx.PlaySound("bounce");
		}
		else
			GotoFrame(landingFrame);
	}

	mustTurnAround = ((framePointer->frameProp.state == state::stand || framePointer->frameProp.state == state::crouch) &&
		(root.x < target->root.x && side == -1 || root.x > target->root.x && side == 1));

	SeqFun();

	if(friction &&((vel.x.value < 0 && accel.x.value < 0) || (vel.x.value > 0 && accel.x.value > 0))) //Pushback can't accelerate. Only slow down.
	{
		vel.x.value = 0;
		accel.x.value = 0;
	}

	if (touchedWall != 0 && (pushTimer > 0)) //Push opponent away
	{
		target->root.x -= vel.x;
	}
	
	Translate(vel);
	vel += accel;

	if (root.y < floorPos) //Check collision with floor
	{
		root.y.value = floorPos.value-1;
	}

	if(blockTime > 0)
	{
		--blockTime;
		return true;
	}

	--pushTimer;
	--frameDuration;
	++totalSubframeCount;
	++subframeCount;

	return true;
}

bool Character::TurnAround(int sequence)
{
	//Second condition fixes a bug where grabbing the wall while backturned doesn't let you turn around.
	if (GetSide() < 0 && (root.x < target->root.x || (touchedWall==-1 && root.x <= target->root.x))) 
	{
		mustTurnAround = false;
		SetSide(1);
		GotoSequence(sequence);
		return true;
	}
	else if (GetSide() > 0 && (root.x > target->root.x || (touchedWall==1 && root.x >= target->root.x)))
	{
		mustTurnAround = false;
		SetSide(-1);
		GotoSequence(sequence);
		return true;
	}
	return false;
}

void Character::Input(InputBuffer &keyPresses, const ChargeState &charges, CommandInputs &cmd)
{	
	int inputSide = GetSide();
	if(mustTurnAround)
		inputSide = -inputSide;

	MotionData command;
	
	auto fp = framePointer;
	if(hurtSeq >= 0)
	{
		fp = &(*sequences)[hurtSeq].frames[0];
	}
	auto &prop = fp->frameProp;
	auto flags = prop.flags;
	
	bool cancellable[2]; //Normal, special
	for(int i = 0; i < 2; ++i)
	{
		switch(prop.cancelType[i]){ //TODO: Generate flags instead?
			case 1: //Always
				cancellable[i] = true;
				break;
			case 2: //Any hit
				cancellable[i] = comboType != none;
				break;
			case 3: //Hurt only
				cancellable[i] = comboType == hurt;
				break;
			case 4: //Block only
				cancellable[i] = comboType == blocked;
				break;
			default: //Never
			case 0:
				cancellable[i] = false;
		}
	}

	CommandInputs::CancelInfo info{
		totalSubframeCount, flags,
		cancellable[0], cancellable[1],
		interruptible,
		currSeq
	};

	if(fp->frameProp.state == state::air)
		command = cmd.ProcessInput(keyPresses, charges, "air", inputSide, info);
	else
		command = cmd.ProcessInput(keyPresses, charges, "ground", inputSide, info);

	if(hitstop)
	{ 
		//TODO: Keep higher priority command
		if(command.seqRef > 0 && command.priority < lastCommand.priority)
			lastCommand = command;
		return;
	}
	else
	{
		if(lastCommand.seqRef > 0)
			command = lastCommand;
		lastCommand = {};
	}

	//Finally perform the move, if there's any.
	if(command.seqRef != -1)
	{
		GotoSequenceMayTurn(command.seqRef);
		if(command.flags & CommandInputs::wipeBuffer) 
			keyPresses.back() |= key::buf::CUT;

		if(command.flags & CommandInputs::interruptible) 
			interruptible = true;
	}	
}

Player::Player(BattleInterface& scene):
scene(scene)
{
	//updateList.push_back((Actor*)this);
}

Player::~Player()
{
	delete charObj;
}

Player::Player(int side, std::string charFile, BattleInterface& scene, int paletteSlot, bool ai):
scene(scene)
{
	Load(side, charFile, paletteSlot, ai);
}

void Player::Load(int side, std::string charFile, int paletteSlot, bool ai)
{
	charObj = new Character(FixedPoint(50*-side), side, scene, lua, sequences, newChildren);
	charObj->paletteIndex = paletteSlot;
	
	aiPlayer = ai;
	if(!ScriptSetup(ai))
		abort();
	cmd.LoadFromLua("data/char/vaki/moves.lua", lua);
	LoadSequences(sequences, charFile, lua); //Sequences refer to script.

	charObj->GotoSequence(0);
	charObj->GotoFrame(0);
	
	sol::protected_function init = lua["_init"];
	if(init.get_type() == sol::type::function)
		init();
}

void Player::SetState(PlayerStateCopy &state)
{
	lua["G"] = state.luaState;
/* 	for(auto &e : state.luaState)
	{
		lua[e.first] = e.second;
	} */
	*charObj = *state.charObj;
	children = state.children;
	target = state.target;
	chargeState = state.chargeState;
	lastKey[0] = state.lastKey[0];
	lastKey[1] = state.lastKey[1];
	priority = state.priority;
}

sol::object DeepCopy(sol::object obj, sol::table seen, sol::state &lua)
{
	auto type = obj.get_type();
	if(type == sol::type::nil)
		return sol::nil;
	
	if(!seen.valid())
	{
		seen = lua.create_table();
	}
	else if(seen[obj].get_type() != sol::type::nil && seen[obj].get_type() != sol::type::none)
	{
		auto test = seen[obj].get_type();
		return seen[obj];
	}
	
	if(type == sol::type::table)
	{
		seen[obj] = lua.create_table();
		auto st = seen[obj];
		for(auto &o : obj.as<sol::table>())
		{
			auto first = DeepCopy(o.first, seen, lua);
			auto second = DeepCopy(o.second, seen, lua);
			seen[obj][first] = second;
		}
		return seen[obj];
	}
	else
		return obj;
}

PlayerStateCopy Player::GetStateCopy()
{
 	Character *p = new Character(0, 0, scene, lua, sequences, children);
	*p = *charObj;
	PlayerStateCopy copy{
		lua.create_table(),
		nullptr,
		children,
		target,
		chargeState,
		{lastKey[0], lastKey[1]},
		priority
	};
	copy.charObj.reset(p);
	//copy.luaState.create();
	copy.luaState = DeepCopy(lua["G"], sol::nil, lua);
	return copy;
}

bool Player::ScriptSetup(bool ai)
{
	lua.open_libraries(sol::lib::base, sol::lib::math);
	auto global = lua["global"].get_or_create<sol::table>();
	Actor::DeclareActorLua(lua);

	auto constant = lua["constant"].get_or_create<sol::table>();
	constant["multiplier"] = speedMultiplier;
	auto key = constant["key"].get_or_create<sol::table>();
	key["up"] = key::buf::UP;
	key["down"] = key::buf::DOWN;
	key["left"] = key::buf::LEFT;
	key["right"] = key::buf::RIGHT;
	key["any"] = key::buf::UP | key::buf::DOWN | key::buf::LEFT | key::buf::RIGHT;

	global.set_function("RandomChance", [this](uint64_t percent){
		return (uint64_t)scene.rng.GetU() < (((uint64_t)(std::numeric_limits<uint32_t>::max())+1)/100)*percent;
	});
	global.set_function("RandomInt", [this](uint64_t min, uint32_t max){
		return min + scene.rng.GetU()/(std::numeric_limits<uint32_t>::max() / (max - min + 1) + 1);
	});
	global.set_function("Random", [this](double min, double max) -> double{
		return (double)scene.rng.GetU()/(double)std::numeric_limits<uint32_t>::max() * (max-min) + min;
	});
	global.set_function("PlaySound", [this](std::string audioString){scene.sfx.PlaySound(audioString);});
	global.set_function("DamageTarget", [this](int amount){target->health -= amount;});
	global.set_function("ParticlesNormalRel", [this](int amount, float x, float y){
		scene.particles.PushNormalHit(amount, (float)charObj->root.x+x*charObj->side, float(charObj->root.y)+y);
	});
	global.set_function("GetTarget", [this]()->Actor&{return *charObj->target;});
	global.set_function("SetPriority", [this](int p){
		if(p>0){
			priority = 1; pTarget->priority=0;
		}else{
			priority = 0; pTarget->priority=1;
		}
	});
	global.set_function("GetBlockTime", [this](){return charObj->blockTime;});
	global.set_function("SetBlockTime", [this](int time){charObj->blockTime=time;});
	global.set_function("TurnAround", &Character::TurnAround, charObj);
	global.set_function("GetInput", [this]() -> unsigned int{return this->lastKey[0];});
	global.set_function("GetInputPrev", [this]() -> unsigned int{return this->lastKey[1];});
	global.set_function("GetInputRelative", [this]() -> unsigned int{
		int lastkey = lastKey[0];
		if(charObj->GetSide() > 0)
			return lastkey;
		else{
			int right = lastkey & key::buf::RIGHT;
			int left = lastkey & key::buf::LEFT;
			return (lastkey & ~0xC) | ((!!right) << key::LEFT) | ((!!left) << key::RIGHT);
		}
	});

	lua.create_named_table("G");
	auto result = lua.script_file("data/char/vaki/script.lua");
	if(!result.valid()){
		sol::error err = result;
		std::cerr << "The code has failed to run in script.lua!\n"
		          << err.what() << "\nPanicking and exiting..."
		          << std::endl;
		return false;
	}

	updateFunction = lua["_update"];
	hasUpdateFunction = updateFunction.get_type() == sol::type::function;
	lua["player"] = (Actor*)charObj;


	if(ai)
	{
		auto result = lua.script_file("data/char/vaki/ai.lua");
		if(!result.valid()){
			sol::error err = result;
			std::cerr << "The code has failed to run in ai.lua!\n"
					<< err.what() << "\nPanicking and exiting..."
					<< std::endl;
			return false;
		}

		aiFunction = lua["_ai"];
		if(aiFunction.get_type() != sol::type::function)
		{
			std::cerr << "Player is AI, but there's no _main function in the ai script" << std::endl;
			return false;
		}
	}
	
	return true;
}

void Player::SetTarget(Player &t)
{
	pTarget = &t;
	target = t.charObj;
	charObj->target = target;
}

void Player::Update(HitboxRenderer *hr)
{
	if(hasUpdateFunction)
	{
		auto result = updateFunction(charObj);
		if(!result.valid())
		{
			sol::error err = result;
			std::cerr << err.what() << "\n";
		}
	}

	if(charObj->Update() && hr)
		charObj->SendHitboxData(*hr);
	for(auto it = children.begin(); it != children.end();)
	{
		if((*it).Update())
		{
			if(hr)
				(*it).SendHitboxData(*hr);
			++it;
		}
		else
		{
			it = children.erase(it);
		}
	}

	for(auto it = newChildren.begin(); it != newChildren.end();)
	{
		if((*it).Update())
		{
			children.push_back(std::move(*it));
			if(hr)
				(*it).SendHitboxData(*hr);
			++it;
		}
		else
		{
			it = newChildren.erase(it);
		}
	}
	newChildren.clear();
}

int Player::FillDrawList(DrawList &dl)
{
	for(int i = children.size()-1; i >= 0; --i)
	{
		dl.v[dl.middle] = &children[i];
		++dl.middle;
	}
	dl.v[dl.middle] = charObj;
	dl.middle += 1;
	return dl.middle-1;
}

void Player::ProcessInput(InputBuffer inputs)
{
	if(aiPlayer)
	{
		auto result = aiFunction((Actor*)charObj, (Actor*)charObj->target);
		if(!result.valid())
		{
			sol::error err = result;
			std::cerr << err.what() << std::endl;
			return;
		}
		InputBuffer aiInput = result.get<InputBuffer>();
		if(!aiInput.empty())
		{
			auto lastInput = aiInput.size()-1;
			lastKey[1] = lastKey[0];
			lastKey[0] = aiInput[lastInput];

			chargeState.Charge(aiInput.back());
			charObj->Input(aiInput, chargeState, cmd); 
		}
		else
			std::cerr << "Warning: aiFunction returned no inputs!" << std::endl;
	}
	else
	{
		auto lastInput = inputs.size()-1;
		lastKey[1] = lastKey[0];
		lastKey[0] = inputs[lastInput];

		chargeState.Charge(inputs.back());
		charObj->Input(inputs, chargeState, cmd);
	}
}

Point2d<FixedPoint> Player::GetXYCoords()
{
	return charObj->root;
}


float Player::GetHealthRatio()
{
	if (charObj->health < 0)
		charObj->health = 10000;
	return charObj->health * (1.f / 10000.f);
}

void Player::HitCollision(Player &bluePlayer, Player &redPlayer)
{

	std::vector<Actor*> blueList; blueList.resize(bluePlayer.children.size()+1);
	blueList[0] = bluePlayer.charObj;
	for(int i = 1; i < blueList.size(); ++i)
		blueList[i] = &bluePlayer.children[i-1];
	std::vector<Actor*> redList; redList.resize(redPlayer.children.size()+1);
	redList[0] = redPlayer.charObj;
	for(int i = 1; i < redList.size(); ++i)
		redList[i] = &redPlayer.children[i-1];

	int blueKey = bluePlayer.lastKey[0];
	int redKey = redPlayer.lastKey[0];
	Player *player[][2] = {{&redPlayer, &bluePlayer},{&bluePlayer, &redPlayer}};
	for(auto blue : blueList)
	{
		for(auto red : redList)
		{
			Actor* mutual[][2] = {{red,blue},{blue,red}};
			const int keys[2] = {redKey, blueKey};
			for(int i = 0; i < 2; i++)
			{
				Actor* const red = mutual[i][0];
				Actor* const blue = mutual[i][1];
				auto result = Actor::HitCollision(*red, *blue);
				if (result.first)
				{
					if(blue->attack.hitStop > blue->hitstop)
						blue->hitstop = blue->attack.hitStop;
					int particleAmount = blue->hitstop*2;
					blue->comboType = red->ResolveHit(keys[i], blue, player[i][0]->aiPlayer);
					if(blue->comboType != Actor::none) //Set hitting player on top.
					{
						player[i][0]->priority = 0;
						player[i][1]->priority = 1;
					}
					if(blue->comboType == Actor::blocked)
					{
						if(blue->attack.blockStop >= 0)
							blue->hitstop = blue->attack.blockStop;
					}
					else if(blue->comboType == Actor::hurt && particleAmount > 0)
					{
						bluePlayer.scene.particles.PushNormalHit(particleAmount, result.second.x, result.second.y);
					}
					else if(blue->comboType == Actor::counter && particleAmount > 0)
					{
						bluePlayer.scene.particles.PushCounterHit(particleAmount, result.second.x, result.second.y);
					}
					blue->hitCount--;
				}
			}
		}
	}
}

void Player::Collision(Player& blue, Player& red)
{
	bool isColliding = false;
	Character *playerOne = blue.charObj, *playerTwo = red.charObj;
	if(playerOne->flags & Actor::noCollision || playerTwo->flags & Actor::noCollision)
		return;

	Rect2d<FixedPoint> colBlue = playerOne->framePointer->colbox;
	Rect2d<FixedPoint> colRed = playerTwo->framePointer->colbox;

	if (playerOne->GetSide() < 0)
		colBlue = colBlue.FlipHorizontal();
	if (playerTwo->GetSide() < 0)
		colRed = colRed.FlipHorizontal();

	colBlue = colBlue.Translate(playerOne->root);
	colRed = colRed.Translate(playerTwo->root);

	isColliding = colBlue.Intersects(colRed);

	if (isColliding)
	{
		const FixedPoint magic(480); //Stage width or large number?

		FixedPoint getAway(0,5);
		FixedPoint getAway1, getAway2;

		if (playerOne->touchedWall != 0 || playerTwo->touchedWall != 0)
			getAway = 1;

		if (playerOne->root.x + playerOne->touchedWall * magic < playerTwo->root.x + playerTwo->touchedWall * magic)
		{
			getAway = getAway * (colBlue.topRight.x - colRed.bottomLeft.x);

			getAway1 = -getAway;
			getAway2 = getAway;
		}
		else
		{
			getAway = getAway * (colRed.topRight.x - colBlue.bottomLeft.x);

			getAway1 = getAway;
			getAway2 = -getAway;
		}

		playerOne->Translate(getAway1, 0);
		playerTwo->Translate(getAway2, 0);
	}

	return;
}