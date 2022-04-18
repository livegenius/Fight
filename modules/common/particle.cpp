#include "particle.h"
#include "xorshift.h"
#include <cmath>

constexpr float pi = 3.1415926535897931;

ParticleGroup::ParticleGroup(XorShift32 &_rng)
{
	rng = &_rng;
}

ParticleGroup::ParticleGroup(const ParticleGroup &p)
{
	*this = p;
}

ParticleGroup& ParticleGroup::operator=(const ParticleGroup &p)
{
	particles = p.particles;
	particleParams = p.particleParams;
	rng = p.rng;
	return *this;
}

ParticleGroup::ParticleGroup(ParticleGroup &&p)
{
	*this = std::move(p);
}

ParticleGroup& ParticleGroup::operator=(ParticleGroup &&p)
{
	particles = std::move(p.particles);
	particleParams = std::move(p.particleParams);
	rng = p.rng;
	return *this;
}

void ParticleGroup::Update()
{
	for(int pi = 0, end = particles.size(); pi < end; ++pi)
	{
		auto& param = particleParams[pi];
		auto& particle = particles[pi];
		while((param.remainingTicks < 0 || particle.scale[0] < 0.01 || particle.scale[1] < 0.01) && pi < end)
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
		//for(int i = 0; i < 4; ++i)
			particle.color[3] *= param.fadeRate;

		particle.scale[0] *= param.growRate[0];
		particle.scale[1] *= param.growRate[1]; 
		float x = particle.sin;
		float y = particle.cos;
		particle.sin = x*param.rotCos - y*param.rotSin;
		particle.cos = x*param.rotSin + y*param.rotCos;
		param.remainingTicks -= 1;
	}
}

constexpr float max32 = static_cast<float>(std::numeric_limits<int32_t>::max());
constexpr float max32u = static_cast<float>(std::numeric_limits<uint32_t>::max());

void ParticleGroup::PushNormalHit(int amount, float x, float y)
{
	constexpr float deceleration = -0.04;
	constexpr float maxSpeed = 12;
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
		particle.scale[0] = 2;
		particle.scale[1] = 2;
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
		particle.scale[0] = 0.35;
		particle.scale[1] = 0.35;

		/* particle.color[0] = (rng->GetU())%128+127;
		particle.color[1] = (rng->GetU())%128+127;
		particle.color[2] = (rng->GetU())%128+127; */

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
	constexpr float maxSpeed = 12;
	//float scale = fmax(amount/20.f, 0.5);
	int start = particles.size();
	particles.resize(start+amount);
	particleParams.resize(start+amount);
/* 	{
		float angle = 2.f*pi*(float)(rng->Get())/max32;
		auto &param = particleParams[start];
		auto &particle = particles[start];
		param = {};
		particle.texId = 0;
		particle.sin = sin(angle);
		particle.cos = cos(angle);
		particle.pos[0] = x;
		particle.pos[1] = y;
		particle.scale[0] = 2.50;
		particle.scale[1] = 8;
		param.growRate[0] = 0.70;
		param.growRate[1] = 0.70;
		param.rotSin = 0;
		param.rotCos = 1;
		param.remainingTicks = 20;
	} */
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
		particle.scale[0] = 1.5f - 1.0*(float)(rng->GetU())/max32u;
		particle.scale[1] = particle.scale[0];
		//memset(particle.color, 0xFF, 4);

		
		param.bounce = true;
		param.rotSin = sin(angle);
		param.rotCos = cos(angle);
		param.vel[0] = particle.scale[0]*maxSpeed*(float)(rng->Get())/max32;
		param.vel[1] = 1.3f*particle.scale[0]*maxSpeed*(float)(rng->GetU())/max32u;
		param.acc[0] = param.vel[0]*deceleration;
		param.acc[1] = -param.vel[1]*deceleration - 0.4*particle.scale[0];
		param.growRate[0] = 1.f - 0.1*(float)(rng->GetU())/max32u;
		param.growRate[1] = param.growRate[0];
		param.remainingTicks = 120;
		param.fadeRate = 0.99f - 0.02*particle.scale[0];
		
	}
}

void ParticleGroup::PushSlash(int amount, float x, float y)
{
	constexpr float deceleration = -0.02;

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
		particle.scale[0] = 2.0;
		particle.scale[1] = 0.17;
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