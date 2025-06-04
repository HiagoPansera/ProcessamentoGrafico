// -----------------------------------------------------------------------------
// INÍCIO DO ARQUIVO: INCLUDES E CONFIGURAÇÕES GERAIS
// -----------------------------------------------------------------------------
// Inclui bibliotecas padrão, OpenGL, GLFW e GLM para gráficos e matemática.
// Define constantes globais para o tamanho da janela, grade e tolerância de cor.
// Define struct Quad para representar cada célula da grade.
// Variáveis globais controlam o estado do jogo e recursos OpenGL.
// -----------------------------------------------------------------------------


#include <iostream>
#include <cmath>
#include <ctime>
#include <sstream>

#include <glad/glad.h>     // Carrega as funções do OpenGL 3.3+
#include <GLFW/glfw3.h>    // Criação de janela e captura de eventos

// GLM para matriz ortográfica e transformações de vértices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

// -----------------------------------------------------------------------------
// Configurações da janela e da grade
// -----------------------------------------------------------------------------
const GLuint WIDTH    = 800;   // Largura da janela em pixels (fixa)
const GLuint HEIGHT   = 600;   // Altura da janela em pixels (fixa)
const GLuint ROWS     = 6;     // Número de linhas de quads na grade
const GLuint COLS     = 8;     // Número de colunas de quads na grade
const GLuint QUAD_W   = 100;   // Largura (pixels) de cada retângulo
const GLuint QUAD_H   = 100;   // Altura  (pixels) de cada retângulo

// Distância máxima no espaço RGB (√( (1-0)^2 + (1-0)^2 + (1-0)^2 )) = √3
// Usaremos para normalizar a distância Euclidiana no cálculo de similaridade.
const float dMax = sqrt(3.0f);

// Tolerância normalizada (0..1). Se (distância Euclidiana / dMax) ≤ 0.2, consideramos “similar”
// e eliminamos o retângulo clicado e todos cujas cores estão dentro dessa faixa.
const float COLOR_TOLERANCE = 0.2f;

// -----------------------------------------------------------------------------
// Aqui representamos cada triângulo da grade
// -----------------------------------------------------------------------------
struct Quad {
    vec3 position;   // Posição em pixels do centro do retângulo
    vec3 dimensions; // Dimensões do retângulo em pixels: (largura, altura, 1.0f)
    vec3 color;      // Cor RGB normalizada em [0,1]
    bool eliminated; // Se true, este retângulo não será desenhado (foi removido)
};

// Matriz fixa de ROWS × COLS de quads
static Quad grid[ROWS][COLS];

// -----------------------------------------------------------------------------
// Estado do jogo
// -----------------------------------------------------------------------------
static int attempts   = 0;    // Número de tentativas válidas
static int score      = 0;    // Pontuação acumulada
static bool gameOver  = false;// Se true, não processamos mais cliques até reiniciar

// Índice linear (0..ROWS*COLS-1) do quad selecionado pelo clique.
// -1 indica que não houve clique válido ou já foi processado.
static int iSelected  = -1;

// Ponteiro global para a janela GLFW, usado para atualizar o título a qualquer momento
static GLFWwindow* gWindow = nullptr;

// IDs OpenGL para o programa de shader e para o VAO (Vertex Array Object)
static GLuint shaderID;
static GLuint VAO;

// Locais dos uniforms dentro do programa de shaders (obtidos após linkar o shader)
static GLint uniColorLoc;
static GLint uniModelLoc;
static GLint uniProjectionLoc;

// -----------------------------------------------------------------------------
// Código-fonte dos Shaders GLSL (Opção B: cor por uniform)
// -----------------------------------------------------------------------------

// Vertex Shader (VS):
// - Recebe atributo “vp” = posição do vértice em espaço local (quad unitário centrado na origem).
// - Usa “projection” (mat4) e “model” (mat4) para transformar para clip-space.
// - Não recebe cor como atributo; cor é enviada ao FS via uniform.
const GLchar* vertexShaderSource = R"glsl(
#version 400 core

layout(location = 0) in vec3 vp;       // Posição local do vértice (x,y,z)

uniform mat4 projection;               // Matriz de projeção ortográfica
uniform mat4 model;                    // Matriz model (translação + escala)

void main()
{
    // Calcula gl_Position = projection × model × (vp,1.0)
    gl_Position = projection * model * vec4(vp, 1.0);
}
)glsl";

// Fragment Shader (FS):
// - Usa uniform “fc” (vec4) como cor uniforme de todos os pixels do quad desenhado.
// - Saída em “frg”.
const GLchar* fragmentShaderSource = R"glsl(
#version 400 core

uniform vec4 fc;    // Cor do fragmento passada como uniform (RGBA)
out vec4 frg;       // Saída de cor do fragment shader

void main()
{
    // Preenche cada fragmento do quad com a cor fornecida
    frg = fc;
}
)glsl";

// -----------------------------------------------------------------------------
// Protótipos de funções
// -----------------------------------------------------------------------------

// Callback de teclado: ESC fecha a janela; R reinicia o jogo.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Callback de clique do mouse: calcula índice da linha/coluna clicada e define iSelected.
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Cria, compila e linka os shaders; retorna ID do programa de shader.
GLuint setupShader();

// Cria VAO/VBO para um quad unitário (centrado na origem, coordenadas em NDC); retorna ID do VAO.
GLuint createQuad();

// Elimina todos os quads cuja cor esteja “próxima” da cor do quad em iSelected.
// Retorna quantos quads foram eliminados nesta chamada.
int eliminarSimilares(float toleranciaNormalized);

// Verifica se ainda existe ao menos um quad não eliminado; retorna true se houver.
bool anyActiveCell();

// Atualiza título da janela com “Score”, “Tentativas” e, se gameOver, adiciona “— FIM DE JOGO! Aperte R para reiniciar.”
void updateWindowTitle();

// Reinicia o jogo:
//  1) Zera attempts, score, gameOver e iSelected.
//  2) Para cada célula na grade:
//       - Define position = (j*QUAD_W + QUAD_W/2, i*QUAD_H + QUAD_H/2, 0) para posicionar no centro.
//       - Define dimensions = (QUAD_W, QUAD_H, 1.0f).
//       - Gera color aleatória em [0,1] para cada canal.
//       - Marca eliminated = false.
//  3) Chama updateWindowTitle() para mostrar “Score: 0 — Tentativas: 0”.
// -----------------------------------------------------------------------------
void resetGame();

/// -----------------------------------------------------------------------------
// FUNÇÃO PRINCIPAL: main()
// -----------------------------------------------------------------------------
// Responsável por inicializar a janela, OpenGL, shaders, grid do jogo e
// executar o loop principal de renderização e eventos.
// O loop principal processa cliques, elimina quads similares, atualiza score
// e redesenha a tela até o usuário fechar a janela.
// -----------------------------------------------------------------------------
int main()
{
    // Inicializa semente aleatória para cores
    srand(static_cast<unsigned int>(time(nullptr)));

    // 1) Inicializa GLFW
    if (!glfwInit())
    {
        cerr << "Falha ao inicializar GLFW\n";
        return -1;
    }

    // 2) Impede redimensionamento da janela (800×600 fixos, encaixe perfeito na grade)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // 3) Cria janela GLFW de WIDTH×HEIGHT pixels com título “Jogo das Cores”
    gWindow = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das Cores", nullptr, nullptr);
    if (!gWindow)
    {
        cerr << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }
    // Define o contexto OpenGL desta janela como o atual
    glfwMakeContextCurrent(gWindow);

    // 4) Registra callbacks de teclado e mouse
    glfwSetKeyCallback(gWindow, key_callback);
    glfwSetMouseButtonCallback(gWindow, mouse_button_callback);

    // 5) Carrega carregador de funções OpenGL (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar GLAD\n";
        glfwTerminate();
        return -1;
    }

    // 6) Define viewport para toda a janela
    int fbW, fbH;
    glfwGetFramebufferSize(gWindow, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    // 7) Compila e linka shaders; guarda ID em shaderID
    shaderID = setupShader();
    glUseProgram(shaderID);

    // 8) Cria VAO/VBO para um quad unitário (−0.5..+0.5 em NDC)
    VAO = createQuad();

    // 9) Obtém locais de uniform no programa de shader:
    //    - uniColorLoc: uniform vec4 fc
    //    - uniModelLoc: uniform mat4 model
    //    - uniProjectionLoc: uniform mat4 projection
    uniColorLoc      = glGetUniformLocation(shaderID, "fc");
    uniModelLoc      = glGetUniformLocation(shaderID, "model");
    uniProjectionLoc = glGetUniformLocation(shaderID, "projection");

    // 10) Prepara matriz de projeção ortográfica (0..800,0..600 → topo-esquerdo como (0,0))
    glm::mat4 projection = glm::ortho(
        0.0f, float(WIDTH),
        float(HEIGHT), 0.0f,
        -1.0f, 1.0f
    );
    glUniformMatrix4fv(uniProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // 11) Inicializa grid e estado do jogo
    resetGame();

    // 12) Loop principal de renderização e eventos
    while (!glfwWindowShouldClose(gWindow))
    {
        // Processa eventos pendentes (teclado, mouse etc.)
        glfwPollEvents();

        // Se houve clique válido (iSelected ≥ 0) e o jogo não acabou, processa eliminação
        if (iSelected >= 0 && !gameOver)
        {
            // Elimina todos os quads similares e recebe quantos foram removidos
            int removedCount = eliminarSimilares(COLOR_TOLERANCE);

            // Se removeu ≥ 1 quad, contabiliza como tentativa válida
            if (removedCount > 0)
            {
                attempts++;                // incrementa tentativas
                int penalty = attempts;    // penalidade = número da tentativa
                score += (removedCount - penalty);
                if (score < 0) score = 0;  // não permite pontuação negativa

                // Imprime no console detalhes da tentativa
                cout << "Tentativa " << attempts
                    << ": removidos " << removedCount
                    << " -> +" << removedCount
                    << " - " << penalty
                    << " = Score: " << score << endl;
            }

            // Se não houver mais quads ativos, sinaliza fim de jogo
            if (!anyActiveCell())
            {
                gameOver = true;
                cout << "FIM DE JOGO! Pontuacao final: " << score << endl;
            }

            // Atualiza título da janela com Score e Tentativas (e mensagem de fim de jogo, se for o caso)
            updateWindowTitle();

            // Limpa seleção para não reprocessar o mesmo clique
            iSelected = -1;
        }

        // Limpa o buffer de cor com fundo preto
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenha todos os quads não eliminados
        glUseProgram(shaderID);
        glBindVertexArray(VAO);

        for (int i = 0; i < int(ROWS); i++)
        {
            for (int j = 0; j < int(COLS); j++)
            {
                if (!grid[i][j].eliminated)
                {
                    // Prepara matriz model: primeiro identidade, depois translate → scale
                    glm::mat4 model = glm::mat4(1.0f);

                    // Translada para o centro do quad em (j*100 + 50, i*100 + 50)
                    model = glm::translate(model, grid[i][j].position);

                    // Escala o quad unitário (−0.5..+0.5) para 100×100 pixels
                    model = glm::scale(model, grid[i][j].dimensions);

                    // Envia a matriz model ao shader (uniform mat4 model)
                    glUniformMatrix4fv(uniModelLoc, 1, GL_FALSE, glm::value_ptr(model));

                    // Envia cor do quad (vec3) como uniform vec4 (r,g,b,1.0f)
                    vec3 c = grid[i][j].color;
                    glUniform4f(uniColorLoc, c.r, c.g, c.b, 1.0f);

                    // Desenha o quad (4 vértices em TRIANGLE_STRIP)
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }
            }
        }

        // Desvincula o VAO (boa prática)
        glBindVertexArray(0);

        // Troca os buffers (duplo-buffer) para exibir o frame desenhado
        glfwSwapBuffers(gWindow);
    }

    // Ao fechar a janela, libera recursos alocados
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwDestroyWindow(gWindow);
    glfwTerminate();
    return 0;
}

// -----------------------------------------------------------------------------
// Função de callback de teclado
//
// Parâmetros:
//  - window: ponteiro para a janela GLFW
//  - key: código da tecla pressionada/liberada
//  - scancode: código de plataforma específico (não usamos aqui)
//  - action: GLFW_PRESS, GLFW_RELEASE ou GLFW_REPEAT
//  - mods: modificadores (Shift, Ctrl, Alt, etc.) (não usados aqui)
//
// Se ESC for pressionado, fecha a janela. Se “R” for pressionado, reinicia o jogo.
// -----------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Fecha a aplicação se pressionar ESC
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    // Reinicia o jogo se pressionar R
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        resetGame();
        cout << "Jogo reiniciado!\n";
    }
}

// -----------------------------------------------------------------------------
// Função de callback de clique do mouse (botão esquerdo)
//
// Processos:
//  1) Verifica se é clique esquerdo e jogo não acabou.
//  2) Obtém coordenadas do cursor (xpos, ypos) em pixels, com (0,0) no canto superior-esquerdo.
//  3) Converte em índices da grade: col = (xpos - 50)/100, row = (ypos - 50)/100.
//  4) Verifica se índices estão dentro dos limites e se o quad ainda não foi eliminado.
//  5) Calcula índice linear para processar a eliminação no loop principal.
// -----------------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !gameOver)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Ajusta subtraindo QUAD_W/2 e QUAD_H/2 antes de dividir,
        // porque grid[i][j].position está no centro de cada quad.
        int col = static_cast<int>((xpos - QUAD_W / 2) / QUAD_W);
        int row = static_cast<int>((ypos - QUAD_H / 2) / QUAD_H);

        // Verifica se índices estão dentro dos limites [0..COLS-1]×[0..ROWS-1]
        if (col >= 0 && col < int(COLS) && row >= 0 && row < int(ROWS))
        {
            // Se esse quad já não estiver eliminado, marca seleção
            if (!grid[row][col].eliminated)
            {
                iSelected = row * COLS + col;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Compila e linka shaders, retornando ID do programa de shader.
// -----------------------------------------------------------------------------
/**
 * Esta função realiza os seguintes passos:
 * 1. Cria, carrega o código-fonte, compila e verifica erros do shader de vértice.
 * 2. Cria, carrega o código-fonte, compila e verifica erros do shader de fragmento.
 * 3. Cria um programa de shader, anexa ambos os shaders compilados e realiza o link.
 * 4. Verifica erros de linkagem do programa.
 * 5. Exclui os shaders individuais após o link, pois não são mais necessários.
 *
 * Em caso de erro de compilação ou linkagem, mensagens detalhadas são impressas no stderr.
 *
 * @return GLuint Identificador do programa de shader criado e vinculado.
 */

GLuint setupShader()
{
    // 1) Vertex Shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);
    GLint success;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(vs, 512, nullptr, log);
        cerr << "ERRO::VERTEX::COMPILATION_FAILED\n" << log << endl;
    }

    /**
 * Cria, compila e vincula shaders para uso em um programa OpenGL.
 *
 * O fragment shader é responsável por calcular a cor final de cada fragmento (pixel) que será desenhado na tela.
 * Ele recebe como entrada as informações interpoladas do vertex shader (como cor, coordenadas de textura, etc.)
 * e produz como saída a cor que será exibida no framebuffer. O fragment shader permite aplicar efeitos de iluminação,
 * texturização, transparência e outros efeitos visuais nos objetos renderizados.
 *
 */
    // 2) Fragment Shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(fs, 512, nullptr, log);
        cerr << "ERRO::FRAGMENT::COMPILATION_FAILED\n" << log << endl;
    }

    // 3) Link Programa
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        cerr << "ERRO::PROGRAM::LINKING_FAILED\n" << log << endl;
    }

    // Deleta shaders individuais após link
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

// -----------------------------------------------------------------------------
// Cria VAO e VBO para um quad unitário (centrado na origem).
//
// A quad é definida como TRIANGLE_STRIP de 4 vértices:
//   (-0.5,  0.5, 0.0)    // topo-esquerda
//   (-0.5, -0.5, 0.0)    // base-esquerda
//   ( 0.5,  0.5, 0.0)    // topo-direita
//   ( 0.5, -0.5, 0.0)    // base-direita
//
// O model matrix será responsável por traduzir e escalar para a posição e tamanho em pixels.
// -----------------------------------------------------------------------------
GLuint createQuad()
{
    GLuint VAO_local, VBO;
    GLfloat vertices[] = {
        //  x      y      z
        -0.5f,  0.5f,  0.0f,  // v0: topo-esquerda
        -0.5f, -0.5f,  0.0f,  // v1: base-esquerda
         0.5f,  0.5f,  0.0f,  // v2: topo-direita
         0.5f, -0.5f,  0.0f   // v3: base-direita
    };

    glGenVertexArrays(1, &VAO_local);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO_local);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // layout(location = 0) in vec3 vp;
        glVertexAttribPointer(
            0,                 // location = 0
            3,                 // 3 componentes (x,y,z)
            GL_FLOAT,          // tipo float
            GL_FALSE,          // não normalizado
            3 * sizeof(GLfloat),// stride
            (void*)0           // offset
        );
        glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return VAO_local;
}

// -----------------------------------------------------------------------------
// Elimina todos os quads cuja cor esteja “próxima” (normalizada ≤ toleranciaNormalized)
// da cor do quad selecionado (iSelected). Retorna quantos quads foram removidos.
//
// Processos:
//  1) Converte índice linear iSelected em (row, col): row = iSelected / COLS, col = iSelected % COLS.
//  2) Lê cor-alvo = grid[row][col].color e marca esse quad como eliminado.
//  3) Para cada outra célula (i,j) não eliminada:
//       calcula distância Euclidiana no espaço RGB:
//         dist = sqrt((r1-r2)^2 + (g1-g2)^2 + (b1-b2)^2)
//         normalizedDist = dist / dMax
//       se normalizedDist ≤ toleranciaNormalized, marca grid[i][j].eliminated = true.
//  4) Retorna removedCount.
// -----------------------------------------------------------------------------
int eliminarSimilares(float toleranciaNormalized)
{
    if (iSelected < 0) return 0;

    int row = iSelected / COLS;
    int col = iSelected % COLS;

    // Marca o quad clicado como eliminado imediatamente
    grid[row][col].eliminated = true;
    vec3 target = grid[row][col].color;

    int removedCount = 1; // Já removemos o clicado

    // Varre toda a grade
    for (int i = 0; i < int(ROWS); i++)
    {
        for (int j = 0; j < int(COLS); j++)
        {
            // Se ainda não estiver eliminado
            if (!grid[i][j].eliminated)
            {
                vec3 c = grid[i][j].color;
                float dist = sqrtf(
                    (c.r - target.r) * (c.r - target.r) +
                    (c.g - target.g) * (c.g - target.g) +
                    (c.b - target.b) * (c.b - target.b)
                );
                float normalizedDist = dist / dMax;
                if (normalizedDist <= toleranciaNormalized)
                {
                    grid[i][j].eliminated = true;
                    removedCount++;
                }
            }
        }
    }

    iSelected = -1;
    return removedCount;
}

// -----------------------------------------------------------------------------
// Verifica se ainda existe ao menos um quad não eliminado.
// Retorna true se houver pelo menos um com eliminated == false.
// -----------------------------------------------------------------------------
bool anyActiveCell()
{
    for (int i = 0; i < int(ROWS); i++)
    {
        for (int j = 0; j < int(COLS); j++)
        {
            if (!grid[i][j].eliminated)
                return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Atualiza o título da janela com informações de Score, Tentativas e, se gameOver,
// adiciona "— FIM DE JOGO! Aperte R para reiniciar."
// -----------------------------------------------------------------------------
void updateWindowTitle()
{
    std::ostringstream oss;
    oss << "Jogo das Cores — Score: " << score
        << " — Tentativas: " << attempts;
    if (gameOver)
    {
        oss << " — FIM DE JOGO! Aperte R para reiniciar.";
    }
    glfwSetWindowTitle(gWindow, oss.str().c_str());
}

// -----------------------------------------------------------------------------
// Reinicia o jogo:
//  1) Zera attempts, score, gameOver e iSelected.
//  2) Para cada célula na grade:
//       - Define position = (j*100 + 50, i*100 + 50, 0) para posicionar no centro do quad.
//       - Define dimensions = (100, 100, 1.0f).
//       - Gera color aleatória em [0,1].
//       - Marca eliminated = false.
//  3) Chama updateWindowTitle() para mostrar Score=0 e Tentativas=0.
// -----------------------------------------------------------------------------
void resetGame()
{
    // 1) Zera variáveis de estado
    attempts  = 0;
    score     = 0;
    gameOver  = false;
    iSelected = -1;

    // 2) Gera cores e define posição central de cada célula
    for (int i = 0; i < int(ROWS); i++)
    {
        for (int j = 0; j < int(COLS); j++)
        {
            Quad& q = grid[i][j];

            // Calcula o centro do quad: (j*100 + 50, i*100 + 50)
            q.position = vec3(
                j * float(QUAD_W) + float(QUAD_W) / 2.0f,
                i * float(QUAD_H) + float(QUAD_H) / 2.0f,
                0.0f
            );

            // Dimensões do quad (100×100)
            q.dimensions = vec3(
                float(QUAD_W),
                float(QUAD_H),
                1.0f
            );

            // Cor aleatória RGB normalizada
            float r = float(rand() % 256) / 255.0f;
            float g = float(rand() % 256) / 255.0f;
            float b = float(rand() % 256) / 255.0f;
            q.color = vec3(r, g, b);

            // Marca como ativo (não eliminado)
            q.eliminated = false;
        }
    }

    // 3) Atualiza o título da janela com valores iniciais
    updateWindowTitle();
}
