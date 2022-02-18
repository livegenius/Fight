#version 460 core
layout(location = 0) in vec2 iTexCoord;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform usampler2D tex;
//uniform sampler2D tex0;
//uniform vec4 mulColor;

void main(void)
{
	float color = texture(tex, iTexCoord/textureSize(tex,0)).r/12.0;
	oColor = vec4(color,color,color,0.9);
}

