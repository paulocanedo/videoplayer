#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "player/VideoPlayer.hpp"
#include "player/VideoDecoder.hpp"

#include <iostream>
#include <chrono>
#include <vector>
using namespace std::chrono;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"

    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
	    "TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    //"in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"
    "mat3 yuv2rgb = mat3(1.0, 0.0, 1.13983, 1.0, -0.39465, -0.58060, 1.0, 2.03211, 0.0);\n"
    "uniform sampler2D textureY;\n"
    "uniform sampler2D textureU;\n"
    "uniform sampler2D textureV;\n"
    "void main()\n"
    "{\n"
    "   vec3 yuv = vec3(texture(textureY, TexCoord).r, texture(textureU, TexCoord * 0.5).r - 0.5, texture(textureV, TexCoord * 0.5).r - 0.5);\n"
    "   vec4 color = texture(textureY, TexCoord);\n"
    //"   vec4 yuv = vec4(texture2D(textureY, TexCoord).r, texture2D(textureU, TexCoord).r - 0.5, texture2D(textureU, TexCoord).g - 0.5, 1.0);\n"
    "   float y = texture(textureY, TexCoord).r;\n"
    "   float u = texture(textureU, TexCoord).r - 0.5;\n"
    "   float v = texture(textureV, TexCoord).r - 0.5;\n"
    "   float r = 1.164 * y + 1.596 * v;"
    "   float g = 1.164 * y - 0.392 * u - 0.813 * v;"
    "   float b = 1.164 * y + 2.017 * u;"

    "   FragColor = vec4(r, g, b, 1.0);\n"
    "}\n\0";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %d - %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

static void pushVertex(std::vector<float> *vertices, const int index, const int numSegments, const float radius) {
    float theta = 2.0f * M_PI * float(index) / float(numSegments);
    float x = radius * cos(theta);
    float y = radius * sin(theta);

    vertices->push_back(x); //x
    vertices->push_back(y); //y
    vertices->push_back(0.0f); //z

    //vertices->push_back(1.0f); vertices->push_back(1.0f); vertices->push_back(1.0f); //color
    vertices->push_back((x + 1.0f) / 2.0f); vertices->push_back(1.0f - (y + 1.0f) / 2.0f); //texture coords
}

static void pushVertexMapped(std::vector<float> *vertices, const float x, const float y) {
    vertices->push_back(x);
    vertices->push_back(y);
    vertices->push_back(0.0f);

    vertices->push_back((x + 1.0f) / 2.0f);
    vertices->push_back(1.0f - (y + 1.0f) / 2.0f);
}

int main(int argc, char** argv) {
//    const char* filename = "/home/paulocanedo/Vídeos/big_buck_bunny_720p_stereo.ogg";
//    const char* filename = "/home/paulocanedo/Vídeos/The World In HDR 4K Demo.mkv";
//    const char* filename = "/home/paulocanedo/Vídeos/videotestsrc.mkv";
    const char* filename = "/home/paulocanedo/Vídeos/LG Tech 4K Demo.ts";
//    const char* filename = "/home/paulocanedo/Vídeos/Big_Buck_Bunny_1080_10s_30MB.mp4";
//    const char* filename = "/home/paulocanedo/Vídeos/Big_Buck_Bunny_first_23_seconds_1080p.ogv.720p.webm";

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Falha ao carregar GLAD" << std::endl;
        return -1;
    }

    glfwSwapInterval(0);

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    const int numValuesPerVertice = 5;
    const int numSegments = 360;
    std::vector<float> vertices;
    float radius = 1.0f;
    pushVertexMapped(&vertices, 0.0f, 0.0f);
    for(int i=0; i < numSegments; i++) {
        pushVertex(&vertices, i, numSegments, radius);
    }
    pushVertex(&vertices, 0, numSegments, radius);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (numSegments + 2) * numValuesPerVertice, vertices.data(), GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, numValuesPerVertice * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, numValuesPerVertice * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    const int numTextures = 3;
    uint32_t textures[numTextures];
    glGenTextures(numTextures, &textures[0]);

    for(int i=0; i<numTextures; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    VideoPlayer player;
    if(!player.init()) {
        std::cerr << "Falha ao inicializar o player\n";
    }

    if(!player.load(filename)) {
        std::cerr << "Não foi possível abrir arquivo" << filename << std::endl;
    }

    //VideoDecoder* decoder = player.createDecoderFirstVideoStream();
    VideoDecoder decoder(player.getFormatContext());
    decoder.init();
    int width = decoder.getWidth(), height = decoder.getHeight();
    SwsContext* scalerContext = sws_getContext(
        width, height, decoder.getPixelFormat(),
        width, height, AV_PIX_FMT_RGB0, SWS_BILINEAR,
        NULL, NULL, NULL
    );

    if(!scalerContext) {
        std::cerr << "Falha ao criar scaler context.\n";
        return -1;
    }

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    auto start = high_resolution_clock::now();
    double videoStart = glfwGetTime();
    double framePts = 0.0;
    bool flag = false;

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        auto diffTs = glfwGetTime() - videoStart;
        if(diffTs > framePts) {

            framePts = decoder.nextFrame(textures[0], textures[1], textures[2]);
            //if(framePts >= 0) {
                //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bufferY);
                //glGenerateMipmap(GL_TEXTURE_2D);
            //}
        }

        // draw our first triangle
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureY"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureU"), 1);
        glUniform1i(glGetUniformLocation(shaderProgram, "textureV"), 2);

        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / numValuesPerVertice);

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        //std::cout << "__while__ " << duration << "; " << std::endl;
        start = high_resolution_clock::now();
    }

    //free(buffer);

    glfwTerminate();
	return 0;
}

