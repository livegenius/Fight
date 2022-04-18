#ifndef BATTLE_INTERFACE_H_GUARD
#define BATTLE_INTERFACE_H_GUARD

#include "xorshift.h"
#include "particle.h"
#include "camera.h"
#include "audio.h"
#include <unordered_map>

struct BattleInterface
{
	XorShift32 &rng;
	ParticleGroup &particles;
	Camera &view;
	SoundEffects &sfx;
};

#endif /* BATTLE_INTERFACE_H_GUARD */
