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

vec4 IndexedShader()
{
	//return texture(tex0, texCoord);
	int index = int(texelFetch(texI[textureIndex], ivec2(iTexCoord), 0).r);
	vec4 color = texelFetch(palette, ivec2(index+paletteIndex, paletteSlot), 0);
	return color;
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
			oColor = IndexedShader()*mulColor;
			break;
		case 1:
			oColor = TruecolorShader()*mulColor;
			//oColor.a = pow(oColor.a, 2.4);
			break;
	}
}

