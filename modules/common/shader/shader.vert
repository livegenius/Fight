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
	const vec3 positions[3] = vec3[3](
		vec3(1.f,1.f, 0.0f),
		vec3(-1.f,1.f, 0.0f),
		vec3(0.f,-1.f, 0.0f)
	);

	gl_Position = vec4(positions[gl_VertexIndex]*0.2, 1.0) + vec4(gl_InstanceIndex,0,0,0)*0.01;
	//gl_Position = transform * vec4(iPos, 0.0, 1.0);
	oTexCoord = vec2(1);//iTexCoord;
}