#ifndef BATTLE_SCENE_H_GUARD
#define BATTLE_SCENE_H_GUARD

#include "battle_interface.h"
#include "chara.h"
#include "camera.h"
#include "texture.h"
#include "hud.h"
#include "xorshift.h"
#include <particle.h>
#include <shader.h>
#include <ubo.h>
#include <glm/mat4x4.hpp>
#include <SDL_events.h>
#include <ggponet.h>
#include <enet/enet.h>
#undef interface


struct State
{
	XorShift32 rng;
	std::unordered_map<int, ParticleGroup> particleGroups;
	Camera view;
	PlayerStateCopy p1, p2;
	//SaveState():{}
};

class BattleScene
{
private:
	ENetHost *local;
	XorShift32 rng;
	std::unordered_map<int, ParticleGroup> particleGroups;
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
	HitboxRenderer hr;
	Player::DrawList drawList;
	Player* players[2];
	GGPOPlayerHandle playerHandle[2];
	GGPOSession *ggpo = nullptr;
	glm::mat4 viewMatrix; //Camera view.

public:
	BattleScene(ENetHost *local);
	~BattleScene();
	void SaveState(State &state);
	void LoadState(State &state);

	int PlayLoop(bool replay, int playerId, const std::string &address);

private:
	std::vector<Texture> activeTextures;

	//Renderer stuff
	glm::mat4 projection;
	Ubo uniforms;
	Shader defaultS;
	unsigned int paletteId;

	void SetModelView(glm::mat4 &view);
	void SetModelView(glm::mat4 &&view);
	bool KeyHandle(const SDL_KeyboardEvent &e); //Returns false if it doesn't handle the event.
	void AdvanceFrame();
	bool SetupGgpo(int playerId, const std::string &address);
};

#endif /* BATTLE_SCENE_H_GUARD */