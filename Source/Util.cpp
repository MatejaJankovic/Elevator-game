#define _CRT_SECURE_NO_WARNINGS
#include "../Header/Util.h"

#include <fstream>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../Header/stb_image.h"

// Autor: Nedeljko Tesanovic
// Opis: pomocne funkcije za zaustavljanje programa, ucitavanje sejdera, tekstura i kursora
// Smeju se koristiti tokom izrade projekta

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        int hotspotX = TextureWidth / 5;
        int hotspotY = TextureHeight / 5;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return nullptr;
    }
}

void setTextureFiltering(unsigned int texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderQuad(unsigned int VAO, unsigned int texture, unsigned int shader, float x, float y, float width, float height, float alpha) {
    glUseProgram(shader);
    
    float model[16] = {
        width, 0.0f, 0.0f, 0.0f,
        0.0f, height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, 0.0f, 1.0f
    };
    
    unsigned int uModelLoc = glGetUniformLocation(shader, "uModel");
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, model);
    
    unsigned int uAlphaLoc = glGetUniformLocation(shader, "uAlpha");
    glUniform1f(uAlphaLoc, alpha);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderColorQuad(unsigned int VAO, unsigned int shader, float x, float y, float width, float height, float r, float g, float b, float a) {
    glUseProgram(shader);
    
    float model[16] = {
        width, 0.0f, 0.0f, 0.0f,
        0.0f, height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, 0.0f, 1.0f
    };
    
    unsigned int uModelLoc = glGetUniformLocation(shader, "uModel");
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, model);
    
    unsigned int uColorLoc = glGetUniformLocation(shader, "uColor");
    glUniform4f(uColorLoc, r, g, b, a);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// 3D Helper Functions
void setShaderMat4(unsigned int shader, const char* name, const Mat4& mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, GL_FALSE, mat.m);
}

void setShaderVec3(unsigned int shader, const char* name, const Vec3& vec) {
    glUniform3f(glGetUniformLocation(shader, name), vec.x, vec.y, vec.z);
}

void setShaderFloat(unsigned int shader, const char* name, float value) {
    glUniform1f(glGetUniformLocation(shader, name), value);
}

unsigned int create3DQuadVAO() {
    // 3D quad with positions, texture coords, and normals (facing +Y by default, floor)
    float vertices[] = {
        // positions          // tex coords  // normals
        -0.5f, 0.0f, -0.5f,   0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   1.0f, 0.0f,    0.0f, 1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,   1.0f, 1.0f,    0.0f, 1.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,   0.0f, 1.0f,    0.0f, 1.0f, 0.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normals
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

unsigned int createWallVAO() {
    // Wall quad facing -Z
    float vertices[] = {
        // positions          // tex coords  // normals
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,    0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f,    0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,    0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

unsigned int createCubeVAO() {
    float vertices[] = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        // Left face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
        // Right face
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f, -1.0f, 0.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,       // back
        4, 5, 6, 6, 7, 4,       // front
        8, 9, 10, 10, 11, 8,    // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // bottom
        20, 21, 22, 22, 23, 20  // top
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

void render3DQuad(unsigned int VAO, unsigned int texture, unsigned int shader, const Mat4& model, const Mat4& view, const Mat4& projection, const Vec3& lightPos, const Vec3& viewPos) {
    glUseProgram(shader);
    
    setShaderMat4(shader, "uModel", model);
    setShaderMat4(shader, "uView", view);
    setShaderMat4(shader, "uProjection", projection);
    setShaderVec3(shader, "uLightPos", lightPos);
    setShaderVec3(shader, "uViewPos", viewPos);
    setShaderVec3(shader, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
    setShaderFloat(shader, "uAmbientStrength", 0.4f);
    setShaderFloat(shader, "uAlpha", 1.0f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void render3DColorQuad(unsigned int VAO, unsigned int shader, const Mat4& model, const Mat4& view, const Mat4& projection, const Vec3& lightPos, float r, float g, float b, float a) {
    glUseProgram(shader);
    
    setShaderMat4(shader, "uModel", model);
    setShaderMat4(shader, "uView", view);
    setShaderMat4(shader, "uProjection", projection);
    setShaderVec3(shader, "uLightPos", lightPos);
    setShaderVec3(shader, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
    setShaderFloat(shader, "uAmbientStrength", 0.4f);
    glUniform4f(glGetUniformLocation(shader, "uColor"), r, g, b, a);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

OBJModel loadOBJModel(const char* objPath, const char* texturePath) {
    OBJModel model;
    model.VAO = 0;
    model.VBO = 0;
    model.vertexCount = 0;
    model.texture = 0;

    std::vector<float> vertices;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<float> texCoords;
    
    FILE* file = fopen(objPath, "r");
    if (!file) {
        std::cout << "Failed to open OBJ file: " << objPath << std::endl;
        return model;
    }

    char lineHeader[128];
    while (fscanf(file, "%s", lineHeader) != EOF) {
        if (strcmp(lineHeader, "v") == 0) {
            Vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            positions.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            Vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            float u, v;
            fscanf(file, "%f %f\n", &u, &v);
            texCoords.push_back(u);
            texCoords.push_back(v);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            char line[256];
            fgets(line, sizeof(line), file);
            
            unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
            
            // Try to read as quad (4 vertices) first
            int matches = sscanf(line, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2],
                &vertexIndex[3], &uvIndex[3], &normalIndex[3]);
            
            if (matches == 12) {
                // Quad face - split into two triangles (0,1,2) and (0,2,3)
                int triangles[2][3] = {{0, 1, 2}, {0, 2, 3}};
                
                for (int t = 0; t < 2; t++) {
                    for (int i = 0; i < 3; i++) {
                        int idx = triangles[t][i];
                        Vec3 pos = positions[vertexIndex[idx] - 1];
                        vertices.push_back(pos.x);
                        vertices.push_back(pos.y);
                        vertices.push_back(pos.z);
                        
                        if (uvIndex[idx] - 1 < texCoords.size() / 2) {
                            vertices.push_back(texCoords[(uvIndex[idx] - 1) * 2]);
                            vertices.push_back(texCoords[(uvIndex[idx] - 1) * 2 + 1]);
                        } else {
                            vertices.push_back(0.0f);
                            vertices.push_back(0.0f);
                        }
                        
                        if (normalIndex[idx] - 1 < normals.size()) {
                            Vec3 norm = normals[normalIndex[idx] - 1];
                            vertices.push_back(norm.x);
                            vertices.push_back(norm.y);
                            vertices.push_back(norm.z);
                        } else {
                            vertices.push_back(0.0f);
                            vertices.push_back(1.0f);
                            vertices.push_back(0.0f);
                        }
                    }
                }
            } else {
                // Try reading as triangle (3 vertices)
                matches = sscanf(line, "%d/%d/%d %d/%d/%d %d/%d/%d",
                    &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                    &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                    &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
                
                if (matches == 9) {
                    for (int i = 0; i < 3; i++) {
                        Vec3 pos = positions[vertexIndex[i] - 1];
                        vertices.push_back(pos.x);
                        vertices.push_back(pos.y);
                        vertices.push_back(pos.z);
                        
                        if (uvIndex[i] - 1 < texCoords.size() / 2) {
                            vertices.push_back(texCoords[(uvIndex[i] - 1) * 2]);
                            vertices.push_back(texCoords[(uvIndex[i] - 1) * 2 + 1]);
                        } else {
                            vertices.push_back(0.0f);
                            vertices.push_back(0.0f);
                        }
                        
                        if (normalIndex[i] - 1 < normals.size()) {
                            Vec3 norm = normals[normalIndex[i] - 1];
                            vertices.push_back(norm.x);
                            vertices.push_back(norm.y);
                            vertices.push_back(norm.z);
                        } else {
                            vertices.push_back(0.0f);
                            vertices.push_back(1.0f);
                            vertices.push_back(0.0f);
                        }
                    }
                }
            }
        }
    }
    fclose(file);

    model.vertexCount = vertices.size() / 8;

    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);

    glBindVertexArray(model.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    if (texturePath) {
        model.texture = loadImageToTexture(texturePath);
        setTextureFiltering(model.texture);
    }

    std::cout << "Loaded OBJ model: " << objPath << " with " << model.vertexCount << " vertices" << std::endl;
    return model;
}

void renderOBJModel(const OBJModel& model, unsigned int shader, const Mat4& modelMatrix, const Mat4& view, const Mat4& projection, const Vec3& lightPos, const Vec3& viewPos) {
    if (model.VAO == 0) return;

    glUseProgram(shader);
    
    setShaderMat4(shader, "uModel", modelMatrix);
    setShaderMat4(shader, "uView", view);
    setShaderMat4(shader, "uProjection", projection);
    setShaderVec3(shader, "uLightPos", lightPos);
    setShaderVec3(shader, "uViewPos", viewPos);
    setShaderVec3(shader, "uLightColor", Vec3(1.0f, 0.95f, 0.9f));  // Warm white light
    setShaderFloat(shader, "uAmbientStrength", 0.15f);  // Low ambient for realistic shadows
    setShaderFloat(shader, "uConstant", 1.0f);
    setShaderFloat(shader, "uLinear", 0.09f);
    setShaderFloat(shader, "uQuadratic", 0.032f);
    setShaderFloat(shader, "uAlpha", 1.0f);
    
    if (model.texture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model.texture);
    }
    
    glBindVertexArray(model.VAO);
    glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);
    glBindVertexArray(0);
}
