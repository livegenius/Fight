#version 460 core
layout(location = 0) in vec2 iTexCoord;

layout(location = 0) out vec4 oColor;

//uniform sampler2D tex0;
//uniform vec4 mulColor;

void main(void)
{
	//vec4 color = texture(tex0, iTexCoord/textureSize(tex0,0))*mulColor;
	oColor = vec4(1);//color; 
}

