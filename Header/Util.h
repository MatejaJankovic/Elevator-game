#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);

void setTextureFiltering(unsigned int texture);
void renderQuad(unsigned int VAO, unsigned int texture, unsigned int shader, float x, float y, float width, float height, float alpha = 1.0f);
void renderColorQuad(unsigned int VAO, unsigned int shader, float x, float y, float width, float height, float r, float g, float b, float a);