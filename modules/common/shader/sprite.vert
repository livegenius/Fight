#version 460 core
layout (location = 0) in ivec2 iPos;
layout (location = 1) in ivec2 iTexCoord;

layout (location = 0) out vec2 oTexCoord;

layout (push_constant) uniform Common
{
	mat4 transform;
	int shaderType;
	int textureIndex;
	int paletteSlot;
	int paletteIndex;
	vec4 mulColor;
};

void main()
{
	gl_Position = transform * vec4(iPos, 0.0, 1.0);
	oTexCoord = vec2(iTexCoord);
}