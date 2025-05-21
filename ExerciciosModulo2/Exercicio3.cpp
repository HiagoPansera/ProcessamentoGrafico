#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// Struct Triangle com posição e cor
struct Triangle {
    vec2 position;
    vec3 color;
};

// Variáveis globais
vector<Triangle> triangles;
GLuint shaderID;
GLuint triangleVAO;
GLint modelLoc, colorLoc, projectionLoc;
GLFWwindow* window;
int windowWidth = 800, windowHeight = 600;

// Shaders
const char* vertexShaderSource = R"glsl(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(position, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 400
uniform vec3 inputColor;
out vec4 color;
void main() {
    color = vec4(inputColor, 1.0);
}
)glsl";

// Função que compila shaders
GLuint compileShader() {
    GLint success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "Erro ao compilar Vertex Shader:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Erro ao compilar Fragment Shader:\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "Erro ao linkar Shader Program:\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria um VAO para um triângulo padrão
GLuint createTriangleVAO() {
    GLfloat vertices[] = {
        -0.1f, -0.1f, 0.0f,
         0.1f, -0.1f, 0.0f,
         0.0f,  0.1f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// Converte coordenadas do mouse para o sistema da OpenGL (-1 a 1)
vec2 screenToGLCoords(double xpos, double ypos) {
    float x = (2.0f * xpos) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * ypos) / windowHeight;
    return vec2(x, y);
}

// Callback de clique do mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        vec2 glPos = screenToGLCoords(xpos, ypos);

        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;

        triangles.push_back({glPos, vec3(r, g, b)});
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Triângulos com Transformação", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Erro ao inicializar GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, windowWidth, windowHeight);

    shaderID = compileShader();
    triangleVAO = createTriangleVAO();

    glUseProgram(shaderID);
    projectionLoc = glGetUniformLocation(shaderID, "projection");
    modelLoc = glGetUniformLocation(shaderID, "model");
    colorLoc = glGetUniformLocation(shaderID, "inputColor");

    mat4 projection = ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(triangleVAO);

        for (const auto& tri : triangles) {
            mat4 model = mat4(1.0f);
            model = translate(model, vec3(tri.position, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glUniform3fv(colorLoc, 1, value_ptr(tri.color));

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
