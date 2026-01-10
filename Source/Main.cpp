// Mateja Janković RA-203/2022
// 3D Elevator Simulator
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../Header/Util.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float TARGET_FPS = 75.0f;
const float FRAME_TIME = 1.0f / TARGET_FPS;
const float PI = 3.14159265359f;

// Floor dimensions in 3D world - DOUBLED SIZE
const float FLOOR_HEIGHT = 6.0f;      // Height of each floor (doubled from 3)
const float FLOOR_WIDTH = 20.0f;      // Width of the building (doubled from 10)
const float FLOOR_DEPTH = 16.0f;      // Depth of each floor (doubled from 8)
const float ELEVATOR_SIZE = 3.0f;     // Elevator cabin size
const int NUM_FLOORS = 8;

// Elevator position - in CORNER (back-right corner)
const float ELEVATOR_X = FLOOR_WIDTH/2 - ELEVATOR_SIZE/2;   // Right side
const float ELEVATOR_Z = -FLOOR_DEPTH/2 + ELEVATOR_SIZE/2;  // Back side

struct Camera {
    Vec3 position;
    float yaw;       // Horizontal rotation
    float pitch;     // Vertical rotation
    float sensitivity;
    float moveSpeed;
    
    Vec3 getForward() const {
        return Vec3(
            cos(pitch) * sin(yaw),
            sin(pitch),
            cos(pitch) * cos(yaw)
        );
    }
    
    Vec3 getRight() const {
        return Vec3(sin(yaw - PI/2), 0, cos(yaw - PI/2));
    }
    
    Mat4 getViewMatrix() const {
        Vec3 forward = getForward();
        Vec3 target = position + forward;
        return Mat4::lookAt(position, target, Vec3(0, 1, 0));
    }
};

struct Button3D {
    Vec3 position;  // Local position relative to elevator (on left wall)
    float width, height;
    unsigned int texture;
    int floorNumber;
    bool isPressed;
};

struct Elevator {
    float y;                    // Current Y position (world coords)
    int currentFloor;
    int targetFloor;
    bool moving;
    bool doorsOpen;
    float doorTimer;
    float speed;
    bool doorExtendUsed;
    std::vector<int> queuedFloors;
};

struct Person {
    Vec3 position;
    bool inElevator;
    int currentFloor;
    float speed;
};

// Global state
bool ventilationActive = false;
Camera camera;
Elevator* globalElevator = nullptr;
Person* globalPerson = nullptr;
std::vector<Button3D>* globalButtons = nullptr;
bool firstMouse = true;
double lastMouseX = 0, lastMouseY = 0;
bool keys[1024] = {false};

// Forward declarations
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
float getFloorYPosition(int floor);
void updateElevator(Elevator& elevator, Person& person, float deltaTime);
void updateCamera(Camera& camera, Person& person, float deltaTime);
void addFloorToQueue(Elevator& elevator, int floor);

int main()
{
    if (!glfwInit()) return endProgram("GLFW nije uspeo da se inicijalizuje.");
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "3D Elevator Simulator", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create shaders
    unsigned int shader3D = createShader("Shaders/3d.vert", "Shaders/3d.frag");
    unsigned int colorShader3D = createShader("Shaders/3d.vert", "Shaders/3d_color.frag");
    unsigned int shader2D = createShader("Shaders/basic.vert", "Shaders/basic.frag");

    // Create 3D geometry VAOs
    unsigned int floorVAO = create3DQuadVAO();
    unsigned int wallVAO = createWallVAO();
    unsigned int cubeVAO = createCubeVAO();

    // 2D quad VAO for UI/crosshair
    float vertices2D[] = {
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };
    unsigned int indices2D[] = { 0, 1, 2, 2, 3, 0 };
    
    unsigned int VAO2D, VBO2D, EBO2D;
    glGenVertexArrays(1, &VAO2D);
    glGenBuffers(1, &VBO2D);
    glGenBuffers(1, &EBO2D);
    
    glBindVertexArray(VAO2D);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2D), vertices2D, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2D);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices2D), indices2D, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Load textures
    unsigned int elevatorWallTex = loadImageToTexture("Resources/metal.jpg");
    unsigned int elevatorDoorClosedTex = loadImageToTexture("Resources/zatvorenLift.png");
    unsigned int elevatorDoorOpenTex = loadImageToTexture("Resources/otvorenLift.png");
    unsigned int crosshairTex = loadImageToTexture("Resources/kursor.png");
    
    unsigned int podTex = loadImageToTexture("Resources/pod.png");
    unsigned int plafonTex = loadImageToTexture("Resources/plafon.jpg");
    
    setTextureFiltering(elevatorWallTex);
    setTextureFiltering(elevatorDoorClosedTex);
    setTextureFiltering(elevatorDoorOpenTex);
    setTextureFiltering(crosshairTex);
    setTextureFiltering(podTex);
    setTextureFiltering(plafonTex);

    // Load floor-specific textures (ALL 4 WALLS same texture per floor)
    unsigned int floorTextures[8];
    floorTextures[0] = loadImageToTexture("Resources/podrum.jpg");
    floorTextures[1] = loadImageToTexture("Resources/prizemlje.jpg");
    floorTextures[2] = loadImageToTexture("Resources/prviSprat.jpg");
    floorTextures[3] = loadImageToTexture("Resources/drugiSprat.jpg");
    floorTextures[4] = loadImageToTexture("Resources/treciSprat.jpg");
    floorTextures[5] = loadImageToTexture("Resources/cetvrtiSprat.jpg");
    floorTextures[6] = loadImageToTexture("Resources/petiSprat.jpg");
    floorTextures[7] = loadImageToTexture("Resources/sestiSprat.jpg");
    for (int i = 0; i < 8; i++) setTextureFiltering(floorTextures[i]);

    // Create button panel - 2 COLUMNS of 6 buttons each, SMALLER size
    std::vector<Button3D> buttons(12);
    float btnSize = 0.15f;      // Smaller buttons
    float btnSpacingY = 0.20f;  // Tighter vertical spacing
    
    // Left and right column positions (in elevator local coords, Z axis)
    float leftColZ = -0.10f;
    float rightColZ = 0.10f;
    
    float btnStartY = 1.8f;  // Start from center height
    
    // LEFT COLUMN (6 buttons: floors 6, 5, 4, 3, 2, 1)
    buttons[7] = {Vec3(0.0f, btnStartY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster6.png"), 7, false};
    buttons[6] = {Vec3(0.0f, btnStartY - btnSpacingY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster5.png"), 6, false};
    buttons[5] = {Vec3(0.0f, btnStartY - 2*btnSpacingY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster4.png"), 5, false};
    buttons[4] = {Vec3(0.0f, btnStartY - 3*btnSpacingY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster3.png"), 4, false};
    buttons[3] = {Vec3(0.0f, btnStartY - 4*btnSpacingY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster2.png"), 3, false};
    buttons[2] = {Vec3(0.0f, btnStartY - 5*btnSpacingY, leftColZ), btnSize, btnSize, loadImageToTexture("Resources/taster1.png"), 2, false};
    
    // RIGHT COLUMN (6 buttons: PR, SU, Open, Close, Stop, Ventilation)
    buttons[1] = {Vec3(0.0f, btnStartY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterPrizemlje.png"), 1, false};
    buttons[0] = {Vec3(0.0f, btnStartY - btnSpacingY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterSuteren.png"), 0, false};
    buttons[8] = {Vec3(0.0f, btnStartY - 2*btnSpacingY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterOtvaranje.png"), -1, false};
    buttons[9] = {Vec3(0.0f, btnStartY - 3*btnSpacingY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterZatvaranje.png"), -2, false};
    buttons[10] = {Vec3(0.0f, btnStartY - 4*btnSpacingY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterStop.png"), -3, false};
    buttons[11] = {Vec3(0.0f, btnStartY - 5*btnSpacingY, rightColZ), btnSize, btnSize, loadImageToTexture("Resources/tasterVentilacija.png"), -4, false};

    for (auto& btn : buttons) setTextureFiltering(btn.texture);

    // Initialize elevator and person
    Elevator elevator = {getFloorYPosition(1), 1, 1, false, false, 0.0f, 3.0f, false, {}};
    Person person = {Vec3(0.0f, getFloorYPosition(1) + 1.7f, 0.0f), false, 1, 5.0f};

    // Initialize camera
    camera = {person.position, PI, 0.0f, 0.002f, 5.0f};

    globalElevator = &elevator;
    globalPerson = &person;
    globalButtons = &buttons;

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    double lastTime = glfwGetTime();
    double accumulator = 0.0;

    // Projection matrix
    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    float aspect = (float)winWidth / (float)winHeight;
    Mat4 projection = Mat4::perspective(PI / 4.0f, aspect, 0.1f, 200.0f);

    Vec3 lightPos(0.0f, 50.0f, 0.0f);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += deltaTime;

        if (accumulator >= FRAME_TIME)
        {
            accumulator -= FRAME_TIME;

            updateElevator(elevator, person, FRAME_TIME);
            updateCamera(camera, person, FRAME_TIME);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Mat4 view = camera.getViewMatrix();

            // ========== RENDER CURRENT FLOOR ==========
            if (!person.inElevator) {
                int floor = person.currentFloor;
                float floorY = floor * FLOOR_HEIGHT;
                
                glUseProgram(shader3D);
                setShaderMat4(shader3D, "uView", view);
                setShaderMat4(shader3D, "uProjection", projection);
                setShaderVec3(shader3D, "uLightPos", lightPos);
                setShaderVec3(shader3D, "uViewPos", camera.position);
                setShaderVec3(shader3D, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
                setShaderFloat(shader3D, "uAmbientStrength", 0.6f);
                setShaderFloat(shader3D, "uAlpha", 1.0f);
                glActiveTexture(GL_TEXTURE0);
                
                // Floor (pod.png)
                Mat4 floorModel = Mat4::translate(Vec3(0.0f, floorY, 0.0f)) * Mat4::scale(Vec3(FLOOR_WIDTH, 1.0f, FLOOR_DEPTH));
                setShaderMat4(shader3D, "uModel", floorModel);
                glBindTexture(GL_TEXTURE_2D, podTex);
                glBindVertexArray(floorVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Ceiling (plafon.jpg)
                Mat4 ceilingModel = Mat4::translate(Vec3(0.0f, floorY + FLOOR_HEIGHT, 0.0f)) * 
                                   Mat4::rotateX(PI) * Mat4::scale(Vec3(FLOOR_WIDTH, 1.0f, FLOOR_DEPTH));
                setShaderMat4(shader3D, "uModel", ceilingModel);
                glBindTexture(GL_TEXTURE_2D, plafonTex);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // ALL 4 WALLS - same texture for current floor
                glBindTexture(GL_TEXTURE_2D, floorTextures[floor]);
                glBindVertexArray(wallVAO);
                
                // Back wall
                Mat4 backWall = Mat4::translate(Vec3(0.0f, floorY + FLOOR_HEIGHT/2, -FLOOR_DEPTH/2)) * 
                               Mat4::scale(Vec3(FLOOR_WIDTH, FLOOR_HEIGHT, 1.0f));
                setShaderMat4(shader3D, "uModel", backWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Front wall
                Mat4 frontWall = Mat4::translate(Vec3(0.0f, floorY + FLOOR_HEIGHT/2, FLOOR_DEPTH/2)) * 
                                Mat4::rotateY(PI) * Mat4::scale(Vec3(FLOOR_WIDTH, FLOOR_HEIGHT, 1.0f));
                setShaderMat4(shader3D, "uModel", frontWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Left wall
                Mat4 leftWall = Mat4::translate(Vec3(-FLOOR_WIDTH/2, floorY + FLOOR_HEIGHT/2, 0.0f)) * 
                               Mat4::rotateY(PI/2) * Mat4::scale(Vec3(FLOOR_DEPTH, FLOOR_HEIGHT, 1.0f));
                setShaderMat4(shader3D, "uModel", leftWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Right wall
                Mat4 rightWall = Mat4::translate(Vec3(FLOOR_WIDTH/2, floorY + FLOOR_HEIGHT/2, 0.0f)) * 
                                Mat4::rotateY(-PI/2) * Mat4::scale(Vec3(FLOOR_DEPTH, FLOOR_HEIGHT, 1.0f));
                setShaderMat4(shader3D, "uModel", rightWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // ========== RENDER ELEVATOR EXTERIOR (cube in corner) ==========
                if (elevator.currentFloor == person.currentFloor) {
                    float cabinY = elevator.y + ELEVATOR_SIZE/2;
                    
                    glBindTexture(GL_TEXTURE_2D, elevatorWallTex);  // Metal for all sides except front
                    
                    // Back wall of elevator (metal)
                    Mat4 elevBack = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z - ELEVATOR_SIZE/2)) * 
                                   Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                    setShaderMat4(shader3D, "uModel", elevBack);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    
                    // Left wall of elevator (metal)
                    Mat4 elevLeft = Mat4::translate(Vec3(ELEVATOR_X - ELEVATOR_SIZE/2, cabinY, ELEVATOR_Z)) * 
                                   Mat4::rotateY(PI/2) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                    setShaderMat4(shader3D, "uModel", elevLeft);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    
                    // Right wall of elevator (metal)
                    Mat4 elevRight = Mat4::translate(Vec3(ELEVATOR_X + ELEVATOR_SIZE/2, cabinY, ELEVATOR_Z)) * 
                                    Mat4::rotateY(-PI/2) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                    setShaderMat4(shader3D, "uModel", elevRight);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    
                    // Front door - render based on door state
                    if (elevator.doorsOpen) {
                        // Doors open - render otvorenLift.png
                        Mat4 elevFront = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z + ELEVATOR_SIZE/2)) * 
                                        Mat4::rotateY(PI) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                        setShaderMat4(shader3D, "uModel", elevFront);
                        glBindTexture(GL_TEXTURE_2D, elevatorDoorOpenTex);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    } else {
                        // Doors closed - render zatvorenLift.png
                        Mat4 elevFront = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z + ELEVATOR_SIZE/2)) * 
                                        Mat4::rotateY(PI) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                        setShaderMat4(shader3D, "uModel", elevFront);
                        glBindTexture(GL_TEXTURE_2D, elevatorDoorClosedTex);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                    // If doors open, don't render front - person can see inside
                }
            }
            else {
                // ========== RENDER ELEVATOR INTERIOR ==========
                float cabinY = elevator.y + ELEVATOR_SIZE/2;
                
                glUseProgram(shader3D);
                setShaderMat4(shader3D, "uView", view);
                setShaderMat4(shader3D, "uProjection", projection);
                setShaderVec3(shader3D, "uLightPos", Vec3(ELEVATOR_X, elevator.y + ELEVATOR_SIZE - 0.5f, ELEVATOR_Z));
                setShaderVec3(shader3D, "uViewPos", camera.position);
                setShaderVec3(shader3D, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
                setShaderFloat(shader3D, "uAmbientStrength", 0.8f);
                setShaderFloat(shader3D, "uAlpha", 1.0f);
                glActiveTexture(GL_TEXTURE0);
                
                glBindTexture(GL_TEXTURE_2D, elevatorWallTex);  // Metal texture
                glBindVertexArray(wallVAO);
                
                // All 4 walls (metal)
                Mat4 elevBackWall = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z - ELEVATOR_SIZE/2)) * 
                                   Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                setShaderMat4(shader3D, "uModel", elevBackWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                Mat4 elevLeftWall = Mat4::translate(Vec3(ELEVATOR_X - ELEVATOR_SIZE/2, cabinY, ELEVATOR_Z)) * 
                                   Mat4::rotateY(PI/2) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                setShaderMat4(shader3D, "uModel", elevLeftWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                Mat4 elevRightWall = Mat4::translate(Vec3(ELEVATOR_X + ELEVATOR_SIZE/2, cabinY, ELEVATOR_Z)) * 
                                    Mat4::rotateY(-PI/2) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                setShaderMat4(shader3D, "uModel", elevRightWall);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Front wall (door) - based on door state
                if (elevator.doorsOpen) {
                    // Doors open - show otvorenLift.png (person can see open door texture)
                    Mat4 elevDoor = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z + ELEVATOR_SIZE/2)) * 
                                   Mat4::rotateY(PI) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                    setShaderMat4(shader3D, "uModel", elevDoor);
                    glBindTexture(GL_TEXTURE_2D, elevatorDoorOpenTex);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                } else {
                    // Doors closed - show zatvorenLift.png
                    Mat4 elevDoor = Mat4::translate(Vec3(ELEVATOR_X, cabinY, ELEVATOR_Z + ELEVATOR_SIZE/2)) * 
                                   Mat4::rotateY(PI) * Mat4::scale(Vec3(ELEVATOR_SIZE, ELEVATOR_SIZE, 1.0f));
                    setShaderMat4(shader3D, "uModel", elevDoor);
                    glBindTexture(GL_TEXTURE_2D, elevatorDoorClosedTex);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
                
                // Floor (pod.png)
                Mat4 elevFloor = Mat4::translate(Vec3(ELEVATOR_X, elevator.y, ELEVATOR_Z)) * 
                                Mat4::scale(Vec3(ELEVATOR_SIZE, 1.0f, ELEVATOR_SIZE));
                setShaderMat4(shader3D, "uModel", elevFloor);
                glBindTexture(GL_TEXTURE_2D, podTex);
                glBindVertexArray(floorVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // Ceiling (metal)
                Mat4 elevCeiling = Mat4::translate(Vec3(ELEVATOR_X, elevator.y + ELEVATOR_SIZE, ELEVATOR_Z)) * 
                                  Mat4::rotateX(PI) * Mat4::scale(Vec3(ELEVATOR_SIZE, 1.0f, ELEVATOR_SIZE));
                setShaderMat4(shader3D, "uModel", elevCeiling);
                glBindTexture(GL_TEXTURE_2D, elevatorWallTex);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                // ========== RENDER BUTTONS ON LEFT WALL ==========
                glBindVertexArray(wallVAO);
                
                for (auto& btn : buttons) {
                    Vec3 btnWorldPos(ELEVATOR_X - ELEVATOR_SIZE/2 + 0.02f, 
                                    elevator.y + btn.position.y, 
                                    ELEVATOR_Z + btn.position.z);
                    
                    // Highlight pressed buttons
                    if (btn.isPressed && btn.floorNumber >= 0) {
                        glUseProgram(colorShader3D);
                        Mat4 highlightModel = Mat4::translate(btnWorldPos + Vec3(0.005f, 0, 0)) * 
                                             Mat4::rotateY(PI/2) * Mat4::scale(Vec3(btn.width + 0.02f, btn.height + 0.02f, 1.0f));
                        setShaderMat4(colorShader3D, "uModel", highlightModel);
                        setShaderMat4(colorShader3D, "uView", view);
                        setShaderMat4(colorShader3D, "uProjection", projection);
                        setShaderVec3(colorShader3D, "uLightPos", Vec3(ELEVATOR_X, elevator.y + ELEVATOR_SIZE, ELEVATOR_Z));
                        setShaderVec3(colorShader3D, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
                        setShaderFloat(colorShader3D, "uAmbientStrength", 0.9f);
                        glUniform4f(glGetUniformLocation(colorShader3D, "uColor"), 1.0f, 1.0f, 0.0f, 1.0f);
                        glBindVertexArray(wallVAO);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                    
                    // Button texture
                    glUseProgram(shader3D);
                    Mat4 btnModel = Mat4::translate(btnWorldPos) * 
                                   Mat4::rotateY(PI/2) * Mat4::scale(Vec3(btn.width, btn.height, 1.0f));
                    setShaderMat4(shader3D, "uModel", btnModel);
                    setShaderMat4(shader3D, "uView", view);
                    setShaderMat4(shader3D, "uProjection", projection);
                    setShaderVec3(shader3D, "uLightPos", Vec3(ELEVATOR_X, elevator.y + ELEVATOR_SIZE, ELEVATOR_Z));
                    setShaderVec3(shader3D, "uViewPos", camera.position);
                    setShaderVec3(shader3D, "uLightColor", Vec3(1.0f, 1.0f, 1.0f));
                    setShaderFloat(shader3D, "uAmbientStrength", 0.9f);
                    setShaderFloat(shader3D, "uAlpha", 1.0f);
                    glBindTexture(GL_TEXTURE_2D, btn.texture);
                    glBindVertexArray(wallVAO);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }

            // Render crosshair (2D overlay)
            glDisable(GL_DEPTH_TEST);
            renderQuad(VAO2D, crosshairTex, shader2D, 0.0f, 0.0f, 0.05f, 0.05f * aspect, 0.7f);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
        }

        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &floorVAO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &VAO2D);
    glDeleteProgram(shader3D);
    glDeleteProgram(colorShader3D);
    glDeleteProgram(shader2D);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
    
    // Call elevator with C key (opens doors if elevator is on same floor)
    if (key == GLFW_KEY_C && action == GLFW_PRESS && globalElevator && globalPerson) {
        if (!globalPerson->inElevator) {
            if (globalElevator->currentFloor != globalPerson->currentFloor) {
                // Call elevator to this floor
                addFloorToQueue(*globalElevator, globalPerson->currentFloor);
            } else {
                // Elevator is here - open doors if they're closed
                if (!globalElevator->doorsOpen && !globalElevator->moving) {
                    globalElevator->doorsOpen = true;
                    globalElevator->doorTimer = 0.0f;
                }
            }
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = (float)(lastMouseX - xpos);
    float yoffset = (float)(lastMouseY - ypos);
    lastMouseX = xpos;
    lastMouseY = ypos;

    camera.yaw += xoffset * camera.sensitivity;
    camera.pitch += yoffset * camera.sensitivity;

    if (camera.pitch > PI/2 - 0.1f) camera.pitch = PI/2 - 0.1f;
    if (camera.pitch < -PI/2 + 0.1f) camera.pitch = -PI/2 + 0.1f;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && globalElevator && globalPerson && globalButtons) {
        if (!globalPerson->inElevator) return;

        Vec3 rayDir = camera.getForward();
        Vec3 rayOrigin = camera.position;

        float closestT = 1000.0f;
        Button3D* hitButton = nullptr;

        for (auto& btn : *globalButtons) {
            Vec3 btnWorldPos(ELEVATOR_X - ELEVATOR_SIZE/2 + 0.02f, 
                            globalElevator->y + btn.position.y, 
                            ELEVATOR_Z + btn.position.z);
            
            float planeX = btnWorldPos.x;
            if (std::abs(rayDir.x) > 0.001f) {
                float t = (planeX - rayOrigin.x) / rayDir.x;
                if (t > 0 && t < closestT) {
                    Vec3 hitPoint = rayOrigin + rayDir * t;
                    if (hitPoint.y >= btnWorldPos.y - btn.height/2 && hitPoint.y <= btnWorldPos.y + btn.height/2 &&
                        hitPoint.z >= btnWorldPos.z - btn.width/2 && hitPoint.z <= btnWorldPos.z + btn.width/2) {
                        closestT = t;
                        hitButton = &btn;
                    }
                }
            }
        }

        if (hitButton) {
            if (hitButton->floorNumber >= 0 && hitButton->floorNumber <= 7) {
                addFloorToQueue(*globalElevator, hitButton->floorNumber);
                hitButton->isPressed = true;
            }
            else if (hitButton->floorNumber == -1) {
                // Open doors button - extend door open time by 5 seconds (only once per opening)
                if (globalElevator->doorsOpen && !globalElevator->doorExtendUsed) {
                    globalElevator->doorTimer = 0.0f;  // Reset timer to add 5 more seconds
                    globalElevator->doorExtendUsed = true;
                }
            }
            else if (hitButton->floorNumber == -2) {
                // Close doors button - immediately close doors
                if (globalElevator->doorsOpen) {
                    globalElevator->doorsOpen = false;
                    globalElevator->doorTimer = 0.0f;
                    globalElevator->doorExtendUsed = false;
                }
            }
            else if (hitButton->floorNumber == -3) {
                // Stop button - stop elevator movement, person cannot exit until elevator reaches a floor
                if (globalElevator->moving) {
                    // Stop the elevator mid-movement
                    globalElevator->moving = false;
                    globalElevator->queuedFloors.clear();
                    
                    // Find nearest floor and go there
                    int nearestFloor = (int)std::round(globalElevator->y / FLOOR_HEIGHT);
                    if (nearestFloor < 0) nearestFloor = 0;
                    if (nearestFloor >= NUM_FLOORS) nearestFloor = NUM_FLOORS - 1;
                    
                    // Add nearest floor to queue so elevator goes there
                    addFloorToQueue(*globalElevator, nearestFloor);
                }
                
                ventilationActive = false;
                
                // Unpress all floor buttons
                for (auto& b : *globalButtons) {
                    if (b.floorNumber >= 0) b.isPressed = false;
                }
            }
            else if (hitButton->floorNumber == -4) {
                // Ventilation button
                ventilationActive = !ventilationActive;
            }
        }
    }
}

float getFloorYPosition(int floor)
{
    return floor * FLOOR_HEIGHT;
}

void updateElevator(Elevator& elevator, Person& person, float deltaTime)
{
    // Door timer - runs whenever doors are open
    if (elevator.doorsOpen) {
        elevator.doorTimer += deltaTime;
        
        float doorOpenTime = elevator.doorExtendUsed ? 10.0f : 5.0f;
        
        if (elevator.doorTimer >= doorOpenTime) {
            elevator.doorsOpen = false;
            elevator.doorTimer = 0.0f;
            elevator.doorExtendUsed = false;
        }
    }

    // Start moving to next floor in queue
    if (!elevator.queuedFloors.empty() && !elevator.moving && !elevator.doorsOpen) {
        elevator.targetFloor = elevator.queuedFloors[0];
        elevator.queuedFloors.erase(elevator.queuedFloors.begin());
        elevator.moving = true;
    }

    // Elevator movement
    if (elevator.moving && !elevator.doorsOpen) {
        float targetY = getFloorYPosition(elevator.targetFloor);
        float direction = (targetY > elevator.y) ? 1.0f : -1.0f;
        elevator.y += direction * elevator.speed * deltaTime;

        // Update person position if in elevator
        if (person.inElevator) {
            person.position.y = elevator.y + 1.7f;
        }

        // Reached target floor
        if (std::abs(elevator.y - targetY) < 0.1f) {
            elevator.y = targetY;
            elevator.currentFloor = elevator.targetFloor;
            elevator.moving = false;
            elevator.doorsOpen = true;  // Open doors when arriving
            elevator.doorTimer = 0.0f;  // Reset timer
            elevator.doorExtendUsed = false;  // Reset extension flag
            
            if (ventilationActive) {
                ventilationActive = false;
            }
            
            // Unpress button for current floor
            if (globalButtons) {
                for (auto& btn : *globalButtons) {
                    if (btn.floorNumber == elevator.currentFloor) {
                        btn.isPressed = false;
                    }
                }
            }
        }
    }
}

void updateCamera(Camera& camera, Person& person, float deltaTime)
{
    Vec3 forward = Vec3(sin(camera.yaw), 0, cos(camera.yaw));
    Vec3 right = Vec3(sin(camera.yaw - PI/2), 0, cos(camera.yaw - PI/2));
    
    Vec3 velocity(0, 0, 0);
    
    if (keys[GLFW_KEY_W]) velocity = velocity + forward;
    if (keys[GLFW_KEY_S]) velocity = velocity - forward;
    if (keys[GLFW_KEY_A]) velocity = velocity - right;
    if (keys[GLFW_KEY_D]) velocity = velocity + right;
    
    if (velocity.length() > 0) {
        velocity = velocity.normalize() * person.speed * deltaTime;
        Vec3 newPos = person.position + velocity;
        
        if (person.inElevator) {
            float margin = 0.4f;
            float elevMinX = ELEVATOR_X - ELEVATOR_SIZE/2 + margin;
            float elevMaxX = ELEVATOR_X + ELEVATOR_SIZE/2 - margin;
            float elevMinZ = ELEVATOR_Z - ELEVATOR_SIZE/2 + margin;
            float elevMaxZ = ELEVATOR_Z + ELEVATOR_SIZE/2 - margin;
            
            // Can only exit if doors are open AND elevator is not moving
            if (globalElevator && globalElevator->doorsOpen && !globalElevator->moving && newPos.z > elevMaxZ) {
                person.inElevator = false;
                person.currentFloor = globalElevator->currentFloor;
                newPos.y = getFloorYPosition(person.currentFloor) + 1.7f;
                newPos.z = ELEVATOR_Z + ELEVATOR_SIZE/2 + 1.0f;
            }
            
            if (person.inElevator) {
                // Constrain movement inside elevator
                if (newPos.x < elevMinX) newPos.x = elevMinX;
                if (newPos.x > elevMaxX) newPos.x = elevMaxX;
                if (newPos.z < elevMinZ) newPos.z = elevMinZ;
                if (newPos.z > elevMaxZ) newPos.z = elevMaxZ;
            }
        } else {
            float floorY = getFloorYPosition(person.currentFloor);
            newPos.y = floorY + 1.7f;
            
            float margin = 0.5f;
            
            // Wall collision
            if (newPos.x < -FLOOR_WIDTH/2 + margin) newPos.x = -FLOOR_WIDTH/2 + margin;
            if (newPos.x > FLOOR_WIDTH/2 - margin) newPos.x = FLOOR_WIDTH/2 - margin;
            if (newPos.z > FLOOR_DEPTH/2 - margin) newPos.z = FLOOR_DEPTH/2 - margin;
            if (newPos.z < -FLOOR_DEPTH/2 + margin) newPos.z = -FLOOR_DEPTH/2 + margin;
            
            // ELEVATOR COLLISION - block movement into elevator area if elevator is on same floor
            if (globalElevator && globalElevator->currentFloor == person.currentFloor) {
                float elevMargin = 0.3f;
                float elevMinX = ELEVATOR_X - ELEVATOR_SIZE/2 - elevMargin;
                float elevMaxX = ELEVATOR_X + ELEVATOR_SIZE/2 + elevMargin;
                float elevMinZ = ELEVATOR_Z - ELEVATOR_SIZE/2 - elevMargin;
                float elevMaxZ = ELEVATOR_Z + ELEVATOR_SIZE/2 + elevMargin;
                
                // Check if trying to walk into elevator area
                bool wouldEnterElevatorArea = (newPos.x > elevMinX && newPos.x < elevMaxX &&
                                              newPos.z > elevMinZ && newPos.z < elevMaxZ);
                
                if (wouldEnterElevatorArea) {
                    // Check if doors are open - can enter through front door only
                    bool inFrontOfDoor = (newPos.z > ELEVATOR_Z + ELEVATOR_SIZE/2 - 0.5f);
                    
                    if (globalElevator->doorsOpen && !globalElevator->moving && inFrontOfDoor) {
                        // Can enter elevator through open doors
                        person.inElevator = true;
                        newPos.x = ELEVATOR_X;
                        newPos.z = ELEVATOR_Z;
                        newPos.y = globalElevator->y + 1.7f;
                    } else {
                        // Blocked by elevator - revert to old position
                        // Calculate collision normal and slide along it
                        Vec3 oldPos = person.position;
                        
                        // Check which side of elevator we're hitting
                        if (std::abs(newPos.x - elevMinX) < 0.1f || std::abs(newPos.x - elevMaxX) < 0.1f) {
                            // Hit side wall - keep old X, allow Z movement
                            newPos.x = oldPos.x;
                        }
                        if (std::abs(newPos.z - elevMinZ) < 0.1f || std::abs(newPos.z - elevMaxZ) < 0.1f) {
                            // Hit front/back wall - keep old Z, allow X movement
                            newPos.z = oldPos.z;
                        }
                        
                        // If still in elevator area after adjustment, full block
                        if (newPos.x > elevMinX && newPos.x < elevMaxX &&
                            newPos.z > elevMinZ && newPos.z < elevMaxZ) {
                            newPos = oldPos; // Complete block
                        }
                    }
                }
            }
        }
        
        person.position = newPos;
    }
    
    camera.position = person.position;
}

void addFloorToQueue(Elevator& elevator, int floor)
{
    if (floor == elevator.currentFloor && !elevator.moving) {
        if (!elevator.doorsOpen) {
            elevator.doorsOpen = true;
            elevator.doorTimer = 0.0f;
        }
        return;
    }
    
    if (std::find(elevator.queuedFloors.begin(), elevator.queuedFloors.end(), floor) == elevator.queuedFloors.end()) {
        elevator.queuedFloors.push_back(floor);
    }
}