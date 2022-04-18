#ifndef PARTICLE_H_GUARD
#define PARTICLE_H_GUARD

#include <functional>
#include <vector>
#include "xorshift.h"

class ParticleGroup
{
public:
	struct Particle{
		float pos[2];
		float scale[2];
		float sin;
		float cos;
		uint32_t texId;
		uint8_t color[4] = {0xFF,0xFF,0xFF,0xFF};
	};

	struct Params{
		float vel[2];
		float acc[2];
		float growRate[2];
		float rotSin; //Rotation speed
		float rotCos;
		int remainingTicks;
		bool bounce = false;
		float fadeRate = 1.f;
	};	
	std::vector<Particle> particles;
	std::vector<Params> particleParams;

	ParticleGroup(XorShift32& rng);
	ParticleGroup& operator=(const ParticleGroup &p);
	ParticleGroup (const ParticleGroup &p);
	ParticleGroup& operator=(ParticleGroup &&p);
	ParticleGroup (ParticleGroup &&p);
	XorShift32 *rng = nullptr;
	void Update();
	void PushNormalHit(int amount, float x, float y);
	void PushCounterHit(int amount, float x, float y);
	void PushSlash(int amount, float x, float y);
};

#endif /* PARTICLE_H_GUARD */
