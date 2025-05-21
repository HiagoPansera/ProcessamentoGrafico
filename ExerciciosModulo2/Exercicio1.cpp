// Cabeçalho
#include <iostream>
#include <vector>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Shaders - Vertex Shader e Fragment Shader
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

const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    uniform vec4 inputColor;
    out vec4 color;
    void main()
    {
        color = inputColor;
    }
)glsl";

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
GLuint setupShader();

// MAIN
int main()
{
    // Inicializa GLFW
    glfwInit();
    // Define versão do OpenGL (3.3 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Cria a janela
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle - OpenGL 3.3+", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Inicializa GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    // Viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compila shaders
    GLuint shaderProgram = setupShader();
    glUseProgram(shaderProgram);

    // Cria triângulo com coordenadas específicas
    vector<GLuint> VAOs;
    VAOs.push_back(createTriangle(-0.6f, -0.5f, 0.0f, 0.5f, 0.6f, -0.5f)); // Triângulo central

    // Uniforms: cor e matrizes
    GLint colorLoc = glGetUniformLocation(shaderProgram, "inputColor");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    mat4 model = mat4(1.0f);
    mat4 projection = ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
    glUniform4f(colorLoc, 0.1f, 0.7f, 0.9f, 1.0f); // azul claro

    // Game Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Limpa a tela
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenha triângulo(s)
        for (auto vao : VAOs)
        {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Finalização
    glfwTerminate();
    return 0;
}

// Callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Compila shaders e retorna o ID do programa
GLuint setupShader()
{
    // Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Checa erros
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Erro ao compilar Vertex Shader:\n" << infoLog << std::endl;
    }

    // Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Erro ao compilar Fragment Shader:\n" << infoLog << std::endl;
    }

    // Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Checa erros de link
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erro ao linkar Shader Program:\n" << infoLog << std::endl;
    }

    // Limpa shaders temporários
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria um triângulo com base em 3 coordenadas 2D (sem transformação)
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
    GLfloat vertices[] = {
        x0, y0, 0.0f, // Vértice 1
        x1, y1, 0.0f, // Vértice 2
        x2, y2, 0.0f  // Vértice 3
    };

    GLuint VAO, VBO;

    // Cria VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Cria VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Configura atributo de posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Limpa bindings
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

