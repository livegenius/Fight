#version 460 core
layout (location = 0) in vec2 iPos;
layout (location = 1) in vec2 iTexCoord;

layout (location = 0) out vec2 oTexCoord;

layout (push_constant) uniform Common
{
	mat4 transform;
};


void main()
{
	gl_Position = transform * vec4(iPos, 0.0, 1.0);
	oTexCoord = iTexCoord;
}