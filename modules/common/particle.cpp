#include "particle.h"
#include "xorshift.h"
#include <cmath>
#include <cassert>

constexpr float pi = 3.1415926535897931;

ParticleGroup::ParticleGroup(XorShift32 &_rng)
{
	rng = &_rng;
	particleTypes.insert({
		{0, {}},
		{1, {}}
	});

	for(auto &[_, type] : particleTypes)
	{
		type.particleParams.reserve(512);
		type.particles.reserve(512);
	}
}

ParticleGroup::ParticleGroup(const ParticleGroup &p)
{
	*this = p;
}

ParticleGroup& ParticleGroup::operator=(const ParticleGroup &p)
{
	particleTypes = p.particleTypes;
	rng = p.rng;
	return *this;
}

ParticleGroup::ParticleGroup(ParticleGroup &&p)
{
	*this = std::move(p);
}

ParticleGroup& ParticleGroup::operator=(ParticleGroup &&p)
{
	particleTypes = std::move(p.particleTypes);
	rng = p.rng;
	return *this;
}

std::vector<ParticleGroup::DrawInfo> ParticleGroup::FillBuffer(uint8_t* buffer, size_t segmentSize, size_t maxSize, uint32_t aligment) const
{
	std::vector<DrawInfo> amountPerId;
	size_t remainingBytes = maxSize;
	for(const auto &[id, data] : particleTypes)
	{
		size_t particlesCopied = 0;
		if(data.particles.empty())
			continue;
		
		auto& particles = data.particles;
		const size_t particleAmount = particles.size(); 

		while(particlesCopied < particleAmount)
		{
			//Size of the amount of particles to copy has to be less than the addressable segment size and can't go beyond the size of the buffer.
			uint32_t particlesToCopy = std::min(std::min(particleAmount-particlesCopied, segmentSize/sizeof(Particle)), remainingBytes/sizeof(Particle));
			uint32_t copiedBytes = particlesToCopy*sizeof(Particle);

			memcpy(buffer, data.particles.data()+particlesCopied, copiedBytes);
			particlesCopied += particlesToCopy;

			//Push draw instance.
			amountPerId.push_back({maxSize-remainingBytes, particlesToCopy, id});

			//Align buffer for next write.
			copiedBytes = (copiedBytes + aligment - 1) & ~(aligment - 1);
			buffer += copiedBytes;
			remainingBytes -= copiedBytes;

			if(remainingBytes == 0)
				goto end;
		}
	}

	end:
	return amountPerId;
}

void ParticleGroup::Update()
{
	for(auto &type : particleTypes)
	{
		auto& particles = type.second.particles;
		auto& particleParams = type.second.particleParams;
		for(int pi = 0, end = particles.size(); pi < end; ++pi)
		{
			auto& param = particleParams[pi];
			auto& particle = particles[pi];
			while((param.remainingTicks < 0 || particle.scale[0] < 0.1 || particle.scale[1] < 0.1) && pi < end)
			{
				param = particleParams.back();
				particle = particles.back();
				particleParams.pop_back();
				particles.pop_back();
				--end;
			}
			if(param.bounce && particle.pos[1] < 32)
			{
				param.vel[1] = -param.vel[1]*0.65;
				particle.pos[1] = 32+(32-particle.pos[1]);
			}
			for(int i = 0; i < 2; ++i)
			{
				particle.pos[i] += param.vel[i];
				param.vel[i] += param.acc[i];
			}
			for(int i = 0; i < 4; ++i)
				particle.color[i] *= param.fadeRate;

			particle.scale[0] *= param.growRate[0];
			particle.scale[1] *= param.growRate[1]; 
			float x = particle.sin;
			float y = particle.cos;
			particle.sin = x*param.rotCos - y*param.rotSin;
			particle.cos = x*param.rotSin + y*param.rotCos;
			param.remainingTicks -= 1;
		}
	}
}

constexpr float max32 = static_cast<float>(std::numeric_limits<int32_t>::max());
constexpr float max32u = static_cast<float>(std::numeric_limits<uint32_t>::max());

void ParticleGroup::PushNormalHit(int amount, float x, float y)
{
	constexpr float deceleration = -0.04;
	constexpr float maxSpeed = 12;

	auto &particles = particleTypes[0].particles;
	auto &particleParams = particleTypes[0].particleParams;

	int start = particles.size();
	particles.resize(start+amount);
	particleParams.resize(start+amount);
	{
		float angle = 2*pi*(float)(rng->GetU())/max32u;
		auto &param = particleParams[start];
		auto &particle = particles[start];
		param = {};
		particle.texId = 1;
		particle.sin = sin(angle);
		particle.cos = cos(angle);
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 80;
		particle.scale[1] = 40;

		particle.color[0] = 255;
		particle.color[1] = 64; 
		particle.color[2] = 64;  
		particle.color[3] = 128;

		param.growRate[0] = 1.025;
		param.growRate[1] = 0.70;
		param.rotSin = 0;
		param.rotCos = 1;
		param.remainingTicks = 12;

		
	}
	for(int i = start+1; i < start+amount; ++i)
	{
		float angle = 0.3f*(float)(rng->Get())/max32;
		auto &param = particleParams[i];
		auto &particle = particles[i];
		
		particle.texId = 1;
		particle.cos = 1;
		particle.sin = 0;
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 14;
		particle.scale[1] = 14;

		particle.color[0] = 255;  //(rng->GetU())%128+127;
		particle.color[1] = 128;  //(rng->GetU())%128+127;
		particle.color[2] = 128;  //(rng->GetU())%128+127; 
		particle.color[3] = 0;

		param.bounce = false;
		param.rotSin = sin(angle);
		param.rotCos = cos(angle);
		param.vel[0] = maxSpeed*(float)(rng->Get())/max32;
		param.vel[1] = maxSpeed*(float)(rng->Get())/max32;
		param.acc[0] = param.vel[0]*deceleration;
		param.acc[1] = param.vel[1]*deceleration - 0.1;
		param.growRate[0] = 0.90f - 0.2*(float)(rng->GetU())/max32u;
		param.growRate[1] = param.growRate[0];
		param.remainingTicks = 20;
	}
}

void ParticleGroup::PushCounterHit(int amount, float x, float y)
{
	constexpr float deceleration = 0.01;
	constexpr float maxSpeed = 2.5;
	//float scale = fmax(amount/20.f, 0.5);
	amount*=5;


	auto &particles = particleTypes[1].particles;
	auto &particleParams = particleTypes[1].particleParams;

	auto &particles0 = particleTypes[0].particles;
	auto &particle0Params = particleTypes[0].particleParams;

	int start = particles.size();
	particles.resize(start+amount);
	particleParams.resize(start+amount);

 	{
		int start0 = particles0.size();
		particles0.resize(start0+1);
		particle0Params.resize(start0+1);
		auto &param = particle0Params[start0];
		auto &particle = particles0[start0];
		param = {};
		particle.sin = 0;
		particle.cos = 1;
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 120;
		particle.scale[1] = 120;
		param.growRate[0] = 0.85;
		param.growRate[1] = 0.85;
		particle.color[0] = 64;
		particle.color[1] = 128;
		particle.color[2] = 255;
		particle.color[3] = 255;
		param.fadeRate = 0.80;
		param.rotSin = 0;
		param.rotCos = 1;
		param.remainingTicks = 20;
	}
	for(int i = start; i < start+amount; ++i)
	{
		float angle = 0.3f*(float)(rng->Get())/max32;
		auto &param = particleParams[i];
		auto &particle = particles[i];
		
		particle.texId = 0;
		particle.cos = 1;
		particle.sin = 0;
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 15.5f - 10.0*(float)(rng->GetU())/max32u;
		particle.scale[1] = particle.scale[0];
		//memset(particle.color, 0xFF, 4);

		particle.color[0] = (rng->GetU())%256;
		particle.color[1] = (rng->GetU())%200+56;
		particle.color[2] = (rng->GetU())%128+128;
		particle.color[3] = 0;

		
		param.bounce = true;
	
		float velx = param.vel[0] = maxSpeed*(float)(rng->Get())/max32;
		float vely = param.vel[1] = 2*maxSpeed*(float)(rng->Get())/max32;
		param.vel[0] *= abs(param.vel[0]);
		param.vel[1] *= abs(param.vel[1]);
		param.acc[0] = param.vel[0]*deceleration;
		param.acc[1] = -abs(param.vel[1])*deceleration - 0.4;
		param.growRate[0] = 0.99f - 0.1*(float)(rng->GetU())/max32u;
		param.growRate[1] = param.growRate[0];
		param.remainingTicks = 120;
		param.fadeRate = param.growRate[0];

		angle += param.vel[0]*0.03;
		param.rotSin = sin(angle);
		param.rotCos = cos(angle);
		
	}
}

void ParticleGroup::PushSlash(int amount, float x, float y)
{
	constexpr float deceleration = -0.02;

	auto &particles = particleTypes[0].particles;
	auto &particleParams = particleTypes[0].particleParams;

	int start = particles.size();
	particles.resize(start+amount);
	particleParams.resize(start+amount);
 	{
		auto &param = particleParams[start];
		auto &particle = particles[start];
		param = {};
		particle.texId = 1;
		particle.sin = 0;
		particle.cos = 1;
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 1.5;
		particle.scale[1] = 1.5;
		param.growRate[0] = 0.80;
		param.growRate[1] = 0.80;
		param.remainingTicks = 10;
		param.rotSin = 0;
		param.rotCos = 1;
	}
	for(int i = start+1; i < start+amount; ++i)
	{
		auto &param = particleParams[i];
		auto &particle = particles[i];
		particle.texId = 1;
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 80.0;
		particle.scale[1] = 6.8;
		param.vel[0] = 15*(float)(rng->Get())/max32;
		param.vel[1] = 15*(float)(rng->Get())/max32;

		float sqvx = sqrt((param.vel[0]*param.vel[0])+(param.vel[1]*param.vel[1]));
		
		//float sqvy = sqrt(param.vel[0]*param.vel[1]);
		particle.cos = param.vel[0] /sqvx;
		particle.sin = param.vel[1] /sqvx;
		param.rotSin = 0;
		param.rotCos = 1;
		param.vel[0] += particle.cos;
		param.vel[1] += particle.sin;
		param.acc[0] = param.vel[0]*deceleration;
		param.acc[1] = param.vel[1]*deceleration;
		param.growRate[0] = 0.95f - 0.04*(float)(rng->GetU())/max32u;
		param.growRate[1] = param.growRate[0];
		param.remainingTicks = 40;
	}
}

