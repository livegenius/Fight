#version 460 core
layout(location = 0) in vec2 iTexCoord;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

void main(void)
{
	oColor = texture(tex, iTexCoord, 0);
}