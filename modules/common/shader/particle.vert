#version 460 core
layout (location = 0) out vec2 oTexCoord;
layout (location = 1) out flat vec4 oColor;

layout (constant_id = 1) const int ParticleLimit = 1;

const float qSize = 1;
const vec2 quad[6] = vec2[6](
	vec2(-qSize, -qSize), vec2(qSize, -qSize), vec2(qSize, qSize),
	vec2(qSize, qSize), vec2(-qSize, qSize), vec2(-qSize, -qSize)
);

const vec2 uv[6] = vec2[6](
	vec2(0, 0), vec2(1, 0), vec2(1, 1),
	vec2(1, 1), vec2(0, 1), vec2(0, 0)
);

layout (push_constant) uniform Common
{
	mat4 transform;
	uint textureId;
};

struct ParticleProp{
	vec2 position;
	vec2 scale;
	vec2 sincos;
	uint id; //unused
	uint colorPacked;
};

layout(set = 0, binding = 0) uniform ObjectBuffer{
	ParticleProp properties[ParticleLimit];
};

void main()
{
	ParticleProp p = properties[gl_VertexIndex/6];
	vec2 scaled = p.scale * quad[gl_VertexIndex%6];
	vec2 rotated = vec2(
		p.sincos.y*scaled.x - p.sincos.x*scaled.y,
		p.sincos.x*scaled.x + p.sincos.y*scaled.y
	);
	
	gl_Position = transform * vec4(rotated+p.position, 0.0, 1.0);
	oTexCoord = uv[gl_VertexIndex%6];
	oColor = unpackUnorm4x8(p.colorPacked);
}