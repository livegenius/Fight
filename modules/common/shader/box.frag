#version 460 core

layout(location = 0) in vec4 iColor;
layout(location = 0) out vec4 oColor;

void main()
{
/* 	vec4 premul = iColor;
	premul.rgb *= premul.a;
	oColor = premul; */
	oColor = iColor;
};
