#version 460 core
layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iColor;

layout (location = 0) out flat vec4 oColor;

layout (push_constant) uniform Common
{
	mat4 transform;
	float alpha;
};

void main()
{
	oColor = vec4(iColor, alpha);
	gl_Position = transform * vec4(iPos, 1);
};
