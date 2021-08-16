#ifndef PARTICLE_H_GUARD
#define PARTICLE_H_GUARD

#include <vector>

struct Particle{
	float pos[2];
	float scale[2];
};

class ParticleGroup
{
private:
	struct ParticleState{
		Particle p;
		float vel[2];
		float acc[2];
		float growRate;
		int remainingTicks;
	};	

	std::vector<ParticleState> particles;

public:
	
	void Update();
	void FillParticleVector(std::vector<Particle> &v);
	void PushNormalHit(int amount, float x, float y);
};

#endif /* PARTICLE_H_GUARD */