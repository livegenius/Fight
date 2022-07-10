#ifndef PARTICLE_H_GUARD
#define PARTICLE_H_GUARD

#include <functional>
#include <unordered_map>
#include <vector>
#include "xorshift.h"

class ParticleGroup
{
public:

	//Goes into uniform buffer
	struct Particle{
		float pos[2];
		float scale[2];
		float sin;
		float cos;
		uint32_t texId;
		uint8_t color[4] = {0xFF,0xFF,0xFF,0xFF};
	};

	//For calculation only.
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

	struct ParticleTypeData{
		std::vector<Particle> particles;
		std::vector<Params> particleParams;
	};
	std::unordered_map<uint32_t, ParticleTypeData> particleTypes;


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

	//Returns list of (amount (up to the segment size), type of particle) for drawing.
	struct DrawInfo{
		size_t bufferOffset;
		uint32_t particleAmount;
		uint32_t particleType;
	};
	std::vector<DrawInfo> FillBuffer(uint8_t* buffer, size_t segmentSize, size_t maxSize, uint32_t aligment) const;
};

#endif /* PARTICLE_H_GUARD */
