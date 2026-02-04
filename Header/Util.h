#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cmath>
#include <vector>

// Math structures for 3D
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    
    float length() const { return sqrt(x*x + y*y + z*z); }
    Vec3 normalize() const { 
        float len = length(); 
        if (len > 0) return Vec3(x/len, y/len, z/len);
        return Vec3(0,0,0);
    }
    
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
    }
    
    static float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }
};

struct Mat4 {
    float m[16];
    
    Mat4() { 
        for (int i = 0; i < 16; i++) m[i] = 0; 
        m[0] = m[5] = m[10] = m[15] = 1;
    }
    
    static Mat4 identity() { return Mat4(); }
    
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 result;
        float tanHalfFov = tan(fov / 2.0f);
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(far + near) / (far - near);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * far * near) / (far - near);
        result.m[15] = 0.0f;
        return result;
    }
    
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalize();
        Vec3 s = Vec3::cross(f, up).normalize();
        Vec3 u = Vec3::cross(s, f);
        
        Mat4 result;
        result.m[0] = s.x; result.m[4] = s.y; result.m[8] = s.z;
        result.m[1] = u.x; result.m[5] = u.y; result.m[9] = u.z;
        result.m[2] = -f.x; result.m[6] = -f.y; result.m[10] = -f.z;
        result.m[12] = -Vec3::dot(s, eye);
        result.m[13] = -Vec3::dot(u, eye);
        result.m[14] = Vec3::dot(f, eye);
        return result;
    }
    
    static Mat4 translate(const Vec3& v) {
        Mat4 result;
        result.m[12] = v.x;
        result.m[13] = v.y;
        result.m[14] = v.z;
        return result;
    }
    
    static Mat4 scale(const Vec3& v) {
        Mat4 result;
        result.m[0] = v.x;
        result.m[5] = v.y;
        result.m[10] = v.z;
        return result;
    }
    
    static Mat4 rotateY(float angle) {
        Mat4 result;
        float c = cos(angle);
        float s = sin(angle);
        result.m[0] = c; result.m[8] = s;
        result.m[2] = -s; result.m[10] = c;
        return result;
    }
    
    static Mat4 rotateX(float angle) {
        Mat4 result;
        float c = cos(angle);
        float s = sin(angle);
        result.m[5] = c; result.m[9] = -s;
        result.m[6] = s; result.m[10] = c;
        return result;
    }
    
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i + j*4] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i + j*4] += m[i + k*4] * other.m[k + j*4];
                }
            }
        }
        return result;
    }
};

// OBJ Model structure
struct OBJModel {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int vertexCount;
    unsigned int texture;
};

// Original 2D functions (kept for UI elements)
int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);

void setTextureFiltering(unsigned int texture);
void renderQuad(unsigned int VAO, unsigned int texture, unsigned int shader, float x, float y, float width, float height, float alpha = 1.0f);
void renderColorQuad(unsigned int VAO, unsigned int shader, float x, float y, float width, float height, float r, float g, float b, float a);

// 3D rendering functions
void setShaderMat4(unsigned int shader, const char* name, const Mat4& mat);
void setShaderVec3(unsigned int shader, const char* name, const Vec3& vec);
void setShaderFloat(unsigned int shader, const char* name, float value);
unsigned int create3DQuadVAO();
unsigned int createWallVAO();
unsigned int createCubeVAO();
void render3DQuad(unsigned int VAO, unsigned int texture, unsigned int shader, const Mat4& model, const Mat4& view, const Mat4& projection, const Vec3& lightPos, const Vec3& viewPos);
void render3DColorQuad(unsigned int VAO, unsigned int shader, const Mat4& model, const Mat4& view, const Mat4& projection, const Vec3& lightPos, float r, float g, float b, float a);

// OBJ Model loading
OBJModel loadOBJModel(const char* objPath, const char* texturePath);
void renderOBJModel(const OBJModel& model, unsigned int shader, const Mat4& modelMatrix, const Mat4& view, const Mat4& projection, const Vec3& lightPos, const Vec3& viewPos);