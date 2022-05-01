#version 460 core
layout(location = 0) in vec2 iTexCoord;

layout(location = 0) out vec4 oColor;

layout (constant_id = 0) const int NumberOfIndexedTextures = 1;
layout (constant_id = 1) const int NumberOfTextures = 1;

layout(set = 0, binding = 0) uniform sampler2D palette;
layout(set = 0, binding = 1) uniform usampler2D texI[NumberOfIndexedTextures];
layout(set = 0, binding = 2) uniform sampler2D tex[NumberOfTextures];
//uniform vec4 mulColor;b

layout (push_constant) uniform Common
{
	mat4 transform;
	int shaderType;
	int textureIndex;
	int paletteSlot;
	int paletteIndex;
	vec4 mulColor;
};

vec4 IndexedSample(ivec2 texCoord)
{
	//return texture(tex0, texCoord);
	int index = int(texelFetch(texI[textureIndex], texCoord, 0).r);
	vec4 color = texelFetch(palette, ivec2(index/* +paletteIndex */, paletteSlot), 0);
	return color;
}

vec4 BilinearSmoothstepSample (vec2 P)
{
	const float sharpness = 0.2;
	vec2 pixelf = P + 0.5;
	
	vec2 frac = fract(pixelf);
	ivec2 pixel = ivec2((floor(pixelf)) - 0.5);

	vec4 C11 = IndexedSample(pixel);
	vec4 C21 = IndexedSample(pixel + ivec2(1, 0));
	vec4 C12 = IndexedSample(pixel + ivec2(0, 1));
	vec4 C22 = IndexedSample(pixel + ivec2(1, 1));

	vec4 x1 = mix(C11, C21, smoothstep(sharpness, 1-sharpness, frac.x));
	vec4 x2 = mix(C12, C22, smoothstep(sharpness, 1-sharpness, frac.x));
	return mix(x1, x2, smoothstep(sharpness, 1-sharpness, frac.y));
}

vec4 TruecolorShader()
{
	return texture(tex[textureIndex], iTexCoord/textureSize(tex[textureIndex],0));
}

void main(void)
{
	switch(shaderType)
	{
		case 0:
			oColor = BilinearSmoothstepSample(iTexCoord)*mulColor;
			break;
		case 1:
			oColor = TruecolorShader()*mulColor;
			break;
	}
}

