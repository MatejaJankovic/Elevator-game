#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../Header/Util.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const float TARGET_FPS = 75.0f;
const float FRAME_TIME = 1.0f / TARGET_FPS;

struct Button {
    float x, y, width, height;
    unsigned int texture;
    int floorNumber;
    bool isPressed;
};

struct Elevator {
    float x, y;
    float width, height;
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
    float x, y;
    float width, height;
    bool inElevator;
    int currentFloor;
    float speed;
    float velocityX;
};

bool ventilationActive = false;
Elevator* globalElevator = nullptr;
Person* globalPerson = nullptr;
std::vector<Button>* globalButtons = nullptr;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
bool checkButtonClick(Button& btn, float ndcX, float ndcY);
float getFloorYPosition(int floor);
void updateElevator(Elevator& elevator, Person& person, float deltaTime);
void updatePerson(Person& person, float deltaTime);
void callElevator(Elevator& elevator, Person& person);
void addFloorToQueue(Elevator& elevator, int floor);
void renderCursor(unsigned int VAO, unsigned int cursorTex, unsigned int shader, double mouseX, double mouseY, int winWidth, int winHeight, bool ventilationOn);

int main()
{
    if (!glfwInit()) return endProgram("GLFW nije uspeo da se inicijalizuje.");
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Elevator Simulator", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shaderProgram = createShader("Shaders/basic.vert", "Shaders/basic.frag");
    unsigned int colorShader = createShader("Shaders/basic.vert", "Shaders/color.frag");
    unsigned int cursorShader = createShader("Shaders/basic.vert", "Shaders/cursor.frag");

    float vertices[] = {
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

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

    unsigned int elevatorClosed = loadImageToTexture("Resources/zatvorenLift.png");
    unsigned int elevatorOpen = loadImageToTexture("Resources/otvorenLift.png");
    unsigned int personTex = loadImageToTexture("Resources/covek.png");
    unsigned int studentInfo = loadImageToTexture("Resources/studentt.png");
    unsigned int cursorTex = loadImageToTexture("Resources/kursor.png");

    setTextureFiltering(elevatorClosed);
    setTextureFiltering(elevatorOpen);
    setTextureFiltering(personTex);
    setTextureFiltering(studentInfo);
    setTextureFiltering(cursorTex);

    std::vector<Button> buttons(12);
    
    float btnStartX = -0.75f;
    float btnStartY = 0.75f;
    float btnSize = 0.12f;
    float btnSpacingY = 0.14f;

    buttons[7] = {btnStartX, btnStartY, btnSize, btnSize, loadImageToTexture("Resources/taster6.png"), 7, false};
    buttons[6] = {btnStartX, btnStartY - btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/taster5.png"), 6, false};
    buttons[5] = {btnStartX, btnStartY - 2*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/taster4.png"), 5, false};
    buttons[4] = {btnStartX, btnStartY - 3*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/taster3.png"), 4, false};
    buttons[3] = {btnStartX, btnStartY - 4*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/taster2.png"), 3, false};
    buttons[2] = {btnStartX, btnStartY - 5*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/taster1.png"), 2, false};
    buttons[1] = {btnStartX, btnStartY - 6*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/tasterPrizemlje.png"), 1, false};
    buttons[0] = {btnStartX, btnStartY - 7*btnSpacingY, btnSize, btnSize, loadImageToTexture("Resources/tasterSuteren.png"), 0, false};
    
    float btnBottomY = btnStartY - 8.5f * btnSpacingY;
    buttons[8] = {btnStartX + 0.1f, btnBottomY, btnSize, btnSize, loadImageToTexture("Resources/tasterOtvaranje.png"), -1, false};
    buttons[9] = {btnStartX - 0.1f, btnBottomY, btnSize, btnSize, loadImageToTexture("Resources/tasterZatvaranje.png"), -2, false};
    buttons[10] = {btnStartX + 0.1f, btnBottomY - btnSpacingY * 0.85f, btnSize, btnSize, loadImageToTexture("Resources/tasterStop.png"), -3, false};
    buttons[11] = {btnStartX - 0.1f, btnBottomY - btnSpacingY * 0.85f, btnSize, btnSize, loadImageToTexture("Resources/tasterVentilacija.png"), -4, false};

    for (auto& btn : buttons) setTextureFiltering(btn.texture);

    Elevator elevator = {0.8f, getFloorYPosition(2), 0.25f, 0.35f, 2, 2, false, false, 0.0f, 0.25f, false, {}};
    Person person = {0.4f, getFloorYPosition(1), 0.15f, 0.28f, false, 1, 0.4f, 0.0f};

    globalElevator = &elevator;
    globalPerson = &person;
    globalButtons = &buttons;

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    double lastTime = glfwGetTime();
    double accumulator = 0.0;

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);

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
            updatePerson(person, FRAME_TIME);

            glClear(GL_COLOR_BUFFER_BIT);

            float floorHeight = 2.0f / 8.0f;
            for (int i = 0; i < 8; i++) {
                float floorY = -1.0f + i * floorHeight + floorHeight / 2.0f;
                renderQuad(VAO, floorTextures[i], shaderProgram, 0.5f, floorY, 1.0f, floorHeight);
            }

            renderColorQuad(VAO, colorShader, -0.75f, 0.0f, 0.45f, 1.85f, 0.75f, 0.75f, 0.75f, 1.0f);

            renderQuad(VAO, studentInfo, shaderProgram, -0.75f, 0.88f, 0.35f, 0.15f, 1.0f);

            for (auto& btn : buttons) {
                if (btn.isPressed && btn.floorNumber >= 0) {
                    renderColorQuad(VAO, colorShader, btn.x, btn.y, btn.width + 0.02f, btn.height + 0.02f, 1.0f, 1.0f, 1.0f, 0.8f);
                }
                renderQuad(VAO, btn.texture, shaderProgram, btn.x, btn.y, btn.width, btn.height);
            }

            unsigned int elevatorTex = elevator.doorsOpen ? elevatorOpen : elevatorClosed;
            renderQuad(VAO, elevatorTex, shaderProgram, elevator.x, elevator.y, elevator.width, elevator.height);

            if (!person.inElevator) {
                renderQuad(VAO, personTex, shaderProgram, person.x, person.y, person.width, person.height);
            } else {
                renderQuad(VAO, personTex, shaderProgram, elevator.x - 0.06f, elevator.y, person.width * 0.7f, person.height * 0.7f);
            }

            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            int winWidth, winHeight;
            glfwGetWindowSize(window, &winWidth, &winHeight);
            renderCursor(VAO, cursorTex, cursorShader, mouseX, mouseY, winWidth, winHeight, ventilationActive && elevator.moving);

            glfwSwapBuffers(window);
        }

        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(colorShader);
    glDeleteProgram(cursorShader);

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

    if (globalPerson && !globalPerson->inElevator) {
        if (key == GLFW_KEY_A) {
            if (action == GLFW_PRESS) {
                globalPerson->velocityX = -globalPerson->speed;
            } else if (action == GLFW_RELEASE) {
                if (globalPerson->velocityX < 0) globalPerson->velocityX = 0.0f;
            }
        }
        if (key == GLFW_KEY_W) {
            if (action == GLFW_PRESS) {
                globalPerson->velocityX = globalPerson->speed;
            } else if (action == GLFW_RELEASE) {
                if (globalPerson->velocityX > 0) globalPerson->velocityX = 0.0f;
            }
        }
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS && globalElevator && globalPerson) {
        if (!globalPerson->inElevator && globalPerson->x >= 0.6f) {
            callElevator(*globalElevator, *globalPerson);
        }
    }

    if (globalPerson && globalPerson->inElevator && globalElevator && globalElevator->doorsOpen) {
        if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            globalPerson->inElevator = false;
            globalPerson->x = globalElevator->x - 0.35f;
            globalPerson->y = globalElevator->y;
            globalPerson->currentFloor = globalElevator->currentFloor;
            globalPerson->velocityX = 0.0f;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && globalElevator && globalPerson && globalButtons) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        float ndcX = (mouseX / width) * 2.0f - 1.0f;
        float ndcY = 1.0f - (mouseY / height) * 2.0f;

        if (!globalPerson->inElevator) return;

        for (auto& btn : *globalButtons) {
            if (checkButtonClick(btn, ndcX, ndcY)) {
                if (btn.floorNumber >= 0 && btn.floorNumber <= 7) {
                    addFloorToQueue(*globalElevator, btn.floorNumber);
                    btn.isPressed = true;
                }
                else if (btn.floorNumber == -1) {
                    if (globalElevator->doorsOpen && !globalElevator->doorExtendUsed) {
                        globalElevator->doorExtendUsed = true;
                    }
                }
                else if (btn.floorNumber == -2) {
                    if (globalElevator->doorsOpen) {
                        globalElevator->doorsOpen = false;
                        globalElevator->doorTimer = 0.0f;
                    }
                }
                else if (btn.floorNumber == -3) {
                    globalElevator->queuedFloors.clear();
                    globalElevator->moving = false;
                    ventilationActive = false;
                    for (auto& b : *globalButtons) {
                        if (b.floorNumber >= 0) b.isPressed = false;
                    }
                }
                else if (btn.floorNumber == -4) {
                    ventilationActive = !ventilationActive;
                }
            }
        }
    }
}

bool checkButtonClick(Button& btn, float ndcX, float ndcY)
{
    return ndcX >= btn.x - btn.width/2 && ndcX <= btn.x + btn.width/2 &&
           ndcY >= btn.y - btn.height/2 && ndcY <= btn.y + btn.height/2;
}

float getFloorYPosition(int floor)
{
    float floorHeight = 2.0f / 8.0f;
    return -1.0f + floor * floorHeight + floorHeight / 2.0f;
}

void updateElevator(Elevator& elevator, Person& person, float deltaTime)
{
    if (elevator.doorsOpen) {
        elevator.doorTimer += deltaTime;
        
        if (!person.inElevator && elevator.currentFloor == person.currentFloor) {
            float personRight = person.x + person.width/2;
            float elevatorLeft = elevator.x - elevator.width/2;
            
            if (personRight >= elevatorLeft - 0.08f && person.velocityX > 0) {
                person.inElevator = true;
                person.x = elevator.x;
                person.velocityX = 0.0f;
            }
        }
        
        if (person.inElevator && elevator.currentFloor != person.currentFloor) {
            person.currentFloor = elevator.currentFloor;
        }

        float doorOpenTime = elevator.doorExtendUsed ? 10.0f : 5.0f;
        
        if (elevator.doorTimer >= doorOpenTime) {
            elevator.doorsOpen = false;
            elevator.doorTimer = 0.0f;
            elevator.doorExtendUsed = false;
        }
    }

    if (!elevator.queuedFloors.empty() && !elevator.moving && !elevator.doorsOpen) {
        elevator.targetFloor = elevator.queuedFloors[0];
        elevator.queuedFloors.erase(elevator.queuedFloors.begin());
        elevator.moving = true;
    }

    if (elevator.moving && !elevator.doorsOpen) {
        float targetY = getFloorYPosition(elevator.targetFloor);
        float direction = (targetY > elevator.y) ? 1.0f : -1.0f;
        elevator.y += direction * elevator.speed * deltaTime;

        if (std::abs(elevator.y - targetY) < 0.02f) {
            elevator.y = targetY;
            elevator.currentFloor = elevator.targetFloor;
            elevator.moving = false;
            elevator.doorsOpen = true;
            elevator.doorTimer = 0.0f;
            
            if (ventilationActive) {
                ventilationActive = false;
            }
            
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

void updatePerson(Person& person, float deltaTime)
{
    if (person.inElevator && globalElevator) {
        person.y = globalElevator->y;
    } else {
        person.x += person.velocityX * deltaTime;
        
        if (person.x < 0.2f) {
            person.x = 0.2f;
            person.velocityX = 0.0f;
        }
        if (person.x > 0.65f) {
            person.x = 0.65f;
            person.velocityX = 0.0f;
        }
    }
}

void callElevator(Elevator& elevator, Person& person)
{
    if (elevator.currentFloor != person.currentFloor) {
        addFloorToQueue(elevator, person.currentFloor);
    } else if (!elevator.doorsOpen) {
        elevator.doorsOpen = true;
        elevator.doorTimer = 0.0f;
    }
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

void renderCursor(unsigned int VAO, unsigned int cursorTex, unsigned int shader, double mouseX, double mouseY, int winWidth, int winHeight, bool ventilationOn)
{
    float ndcX = (mouseX / winWidth) * 2.0f - 1.0f;
    float ndcY = 1.0f - (mouseY / winHeight) * 2.0f;
    
    float cursorSize = 0.05f;
    
    glUseProgram(shader);
    
    float model[16] = {
        cursorSize, 0.0f, 0.0f, 0.0f,
        0.0f, cursorSize, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        ndcX, ndcY, 0.0f, 1.0f
    };
    
    unsigned int uModelLoc = glGetUniformLocation(shader, "uModel");
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, model);
    
    unsigned int uAlphaLoc = glGetUniformLocation(shader, "uAlpha");
    glUniform1f(uAlphaLoc, 1.0f);
    
    unsigned int uVentilationActiveLoc = glGetUniformLocation(shader, "uVentilationActive");
    glUniform1i(uVentilationActiveLoc, ventilationOn ? 1 : 0);
    
    unsigned int uVentilationColorLoc = glGetUniformLocation(shader, "uVentilationColor");
    glUniform3f(uVentilationColorLoc, 1.0f, 0.1f, 0.1f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cursorTex);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}