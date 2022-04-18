#version 460 core
layout (location = 0) in vec2 iTexCoord;
layout (location = 1) in flat uint iTexIndex;
layout (location = 2) in flat vec4 iColor;

layout (location = 0) out vec4 oColor;


layout (constant_id = 0) const int NumberOfTextures = 1;

layout (set = 1, binding = 0) uniform sampler2D tex[NumberOfTextures];

void main(void)
{
	oColor = iColor * texture(tex[iTexIndex], iTexCoord);
}

