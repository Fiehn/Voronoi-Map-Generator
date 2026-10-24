#pragma once
bool initGL();

// Compiles and links a shader program, logs the error and returns 0 on failure.
// Returns a GLuint, kept as unsigned int so this header doesn't need glad
unsigned int compileProgram(const char* vertexSource, const char* fragmentSource);
