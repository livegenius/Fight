#version 460 core
layout(location = 0) in vec2 iTexCoord;

layout(location = 0) out vec4 oColor;

layout (constant_id = 0) const int NumberOfIndexedTextures = 1;
layout (constant_id = 1) const int NumberOfTextures = 1;
layout(set = 0, binding = 0) uniform usampler2D texI[NumberOfIndexedTextures];
//layout(set = 0, binding = 1) uniform sampler2D tex[NumberOfTextures];
//uniform sampler2D tex0;
//uniform vec4 mulColor;

void main(void)
{
	float color = texture(texI[0], iTexCoord/textureSize(texI[0],0)).r/255.0;
	oColor = vec4(vec3(color),1);
}

