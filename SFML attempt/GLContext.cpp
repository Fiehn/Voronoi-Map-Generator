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
