#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <glm/mat4x4.hpp>

unsigned int LoadShader(const char *vertex_path, const char *fragment_path);

class Shader
{
public:
	unsigned int program;

	Shader();
	~Shader();
	Shader(const char *vertex_path, const char *fragment_path);

	void LoadShader(const char *vertex_path, const char *fragment_path);
	void Use();
	unsigned int GetLoc(const char* name);
};

#endif // SHADER_H_INCLUDED
