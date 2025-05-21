#include <iostream>
#include <vector>

// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader
const GLchar* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 position;
    uniform mat4 projection;
    uniform mat4 model;
    void main()
    {
        gl_Position = projection * model * vec4(position, 1.0);
    }
)glsl";

// Fragment Shader
const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    uniform vec4 inputColor;
    out vec4 color;
    void main()
    {
        color = inputColor;
    }
)glsl";

// Funções auxiliares
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
GLuint setupShader();

// Main
int main()
{
    // Inicialização GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criação da janela
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "5 Triangulos - OpenGL", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Erro ao criar a janela" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Inicialização do GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Erro ao carregar GLAD" << std::endl;
        return -1;
    }

    // Viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilação e uso dos shaders
    GLuint shaderProgram = setupShader();
    glUseProgram(shaderProgram);

    // Matrizes uniformes
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "inputColor");

    // Matriz de projeção ortográfica
    mat4 projection = ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

    // Criação de 5 triângulos
    vector<GLuint> VAOs;

    VAOs.push_back(createTriangle(-0.9f, -0.8f, -0.8f, -0.6f, -0.7f, -0.8f)); // Triângulo 1
    VAOs.push_back(createTriangle(-0.4f,  0.0f, -0.3f,  0.2f, -0.2f,  0.0f)); // Triângulo 2
    VAOs.push_back(createTriangle( 0.1f, -0.3f,  0.2f, -0.1f,  0.3f, -0.3f)); // Triângulo 3
    VAOs.push_back(createTriangle( 0.5f,  0.3f,  0.6f,  0.5f,  0.7f,  0.3f)); // Triângulo 4
    VAOs.push_back(createTriangle(-0.1f,  0.5f,  0.0f,  0.7f,  0.1f,  0.5f)); // Triângulo 5

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Limpa a tela
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // fundo escuro
        glClear(GL_COLOR_BUFFER_BIT);

        // Cor para todos os triângulos
        glUniform4f(colorLoc, 0.0f, 0.8f, 0.6f, 1.0f); // verde água

        // Matriz de modelo identidade (sem transformação)
        mat4 model = mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

        // Desenhar cada triângulo
        for (GLuint vao : VAOs)
        {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);

        // Exibe na tela
        glfwSwapBuffers(window);
    }

    // Finaliza GLFW
    glfwTerminate();
    return 0;
}

// Compilação e linkagem dos shaders
GLuint setupShader()
{
    GLint success;
    GLchar infoLog[512];

    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cerr << "Erro Vertex Shader: " << infoLog << endl;
    }

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cerr << "Erro Fragment Shader: " << infoLog << endl;
    }

    // Programa
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Verifica link
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cerr << "Erro ao linkar programa: " << infoLog << endl;
    }

    // Limpeza
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Criação de triângulo com 3 coordenadas
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    GLfloat vertices[] = {
        x0, y0, 0.0f,
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };

    GLuint VAO, VBO;

    // VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Desvincula
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// Tecla ESC para fechar
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
