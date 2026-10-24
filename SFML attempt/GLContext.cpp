#include <glad/glad.h>
#include "GLContext.hpp"
#include "Logger.h"

bool initGL()
{
	// Needs the SFML window's context to be current
	if (!gladLoadGL())
	{
		LOG_ERROR(System, "Failed to load OpenGL functions with glad");
		return false;
	}

	LOG_INFO(System, "OpenGL version: ", std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
	LOG_INFO(System, "GLSL version: ", std::string(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));
	LOG_INFO(System, "Vendor: ", std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR))));
	LOG_INFO(System, "Renderer: ", std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
	return true;
}

namespace {
	GLuint compileShader(GLenum type, const char* source)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint ok = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			char log[1024];
			glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
			LOG_ERROR(Graphics, type == GL_VERTEX_SHADER ? "Vertex shader failed to compile:" : "Fragment shader failed to compile:", std::string(log));
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}
}

unsigned int compileProgram(const char* vertexSource, const char* fragmentSource)
{
	GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (!vs || !fs)
	{
		if (vs) glDeleteShader(vs);
		if (fs) glDeleteShader(fs);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);

	GLint ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		LOG_ERROR(Graphics, "Shader program failed to link:", std::string(log));
		glDeleteProgram(program);
		return 0;
	}
	return program;
}
