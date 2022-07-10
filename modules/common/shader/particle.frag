#version 460 core
layout (location = 0) in vec2 iTexCoord;
layout (location = 1) in flat vec4 iColor;

layout (location = 0) out vec4 oColor;

layout (constant_id = 0) const int NumberOfTextures = 1;

layout (set = 1, binding = 0) uniform sampler2D tex[NumberOfTextures];

layout (push_constant) uniform Common
{
	mat4 transform;
	uint textureId;
};

void main(void)
{
	vec4 t = texture(tex[textureId], iTexCoord);
	//t.rgb *= t.a;

	bvec3 overlayb = greaterThan(iColor.rgb, vec3(0.5));
	vec3 over = 1 - (1-t.rgb)*(1-iColor.rgb)*2;
	
	vec3 below = 2*t.rgb * iColor.rgb;

	vec3 final = mix(below, over, overlayb);
	final *= t.a;
	
	//t *= iColor;
	oColor = vec4(final, t.a*iColor.a);
}

