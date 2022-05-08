#ifndef BATTLE_SCENE_H_GUARD
#define BATTLE_SCENE_H_GUARD

#include <hitbox_renderer.h>
#include "battle_interface.h"
#include "chara.h"
#include "camera.h"
#include "hud.h"
#include "xorshift.h"
#include <particle.h>

#include <glm/mat4x4.hpp>
#include <SDL_events.h>
#include <ggponet.h>
#include <enet/enet.h>
#undef interface


struct State
{
	XorShift32 rng;
	ParticleGroup particles;
	Camera view;
	PlayerStateCopy p1, p2;
	State():particles(rng){}
};

class BattleScene
{
private:
	ENetHost *local;
	XorShift32 rng;
	ParticleGroup particles;
	Camera view{1.55};
	int timer;

	int32_t gameTicks = 0;
	bool pause = false;
	bool step = false;
	bool ready = true;
	BattleInterface interface;
	Player player, player2;
	bool drawBoxes = false;

	SoundEffects sfx;
		
	Player::DrawList drawList;
	Player* players[2];
	GGPOPlayerHandle playerHandle[2];
	GGPOSession *ggpo = nullptr;
	glm::mat4 viewMatrix; //Camera view.

	State savedState;

public:
	BattleScene(ENetHost *local);
	~BattleScene();
	void SaveState(State &state);
	void LoadState(State &state);

	int PlayLoop(bool replay, int playerId, const std::string &address);

private:
	//Renderer stuff
	HitboxRenderer hr;
	glm::mat4 projection;

	void SetModelView(glm::mat4 &view);
	void SetModelView(glm::mat4 &&view);
	bool KeyHandle(const SDL_KeyboardEvent &e); //Returns false if it doesn't handle the event.
	void AdvanceFrame();
	bool SetupGgpo(int playerId, const std::string &address);
};

#endif /* BATTLE_SCENE_H_GUARD */