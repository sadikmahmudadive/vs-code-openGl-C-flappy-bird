#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#include <json/json.h>
#include <fstream>

#include <iostream>
#include <vector>
#include <random>
#include <ctime>

using json = nlohmann::json;

// Link libraries (Windows/MSVC specific)
// #pragma comment(lib, "Libraries/lib/glfw3.lib")
// #pragma comment(lib, "opengl32.lib")

// NOTE: You must compile glad.c alongside this file!
// If you don't have glad.c, you need to generate it from https://glad.dav1d.de/

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Game Constants
const float GRAVITY = -15.0f;
const float JUMP_FORCE = 7.0f;
const float BIRD_SPEED = 3.0f; // Speed at which pipes move towards bird
const float PIPE_SPAWN_X = 10.0f;
const float PIPE_DISTANCE = 6.0f;
const float PIPE_GAP = 2.5f;
const float PIPE_WIDTH = 1.0f;

// Shaders
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;

    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform vec2 texOffset;
    uniform vec2 texScale;

    void main()
    {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        TexCoords = aTexCoords * texScale + texOffset;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;

    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform sampler2D texture1;

    void main()
    {
        // Ambient
        float ambientStrength = 0.5;
        vec3 ambient = ambientStrength * lightColor;
        
        // Diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        // Specular
        float specularStrength = 0.2;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * lightColor * spec;  
        
        vec4 texColor = texture(texture1, TexCoords);
        vec3 result = (ambient + diffuse + specular) * objectColor * texColor.rgb;
        FragColor = vec4(result, 1.0);
    } 
)";

// Cube Vertices (Position + Normal + TexCoords)
float cubeVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

// Global State
bool gameOver = false;
bool gameStarted = false;
int score = 0;
unsigned int whiteTexture;
std::vector<unsigned int> bgTextures;
int currentBgIndex = 0;

struct Bird {
    glm::vec3 position;
    float velocity;
    float size;
    float rotation;

    Bird() : position(0.0f, 0.0f, 0.0f), velocity(0.0f), size(0.5f), rotation(0.0f) {}

    void reset() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = 0.0f;
        rotation = 0.0f;
    }

    void update(float dt) {
        velocity += GRAVITY * dt;
        position.y += velocity * dt;
        
        // Rotation logic
        if (velocity > 0) {
            rotation = 30.0f;
        } else {
            rotation -= 100.0f * dt;
            if (rotation < -90.0f) rotation = -90.0f;
        }
    }

    void jump() {
        velocity = JUMP_FORCE;
        rotation = 30.0f;
    }
};

struct Pipe {
    float x;
    float gapY;
    bool passed;

    Pipe(float startX, float gap) : x(startX), gapY(gap), passed(false) {}
};

std::vector<Pipe> pipes;
std::mt19937 rng;
std::uniform_real_distribution<float> dist(-3.0f, 3.0f);

// Helper to compile shaders
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return id;
}

unsigned int createShaderProgram() {
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    
    int success;
    char infoLog[512];
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return id;
}

// Texture loading helper
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Input callback
Bird bird;
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // R key for restart
    if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        gameOver = false;
        gameStarted = false;
        bird.reset();
        score = 0;
        pipes.clear();
        
        // Change background randomly
        static std::uniform_int_distribution<int> bgDist(0, 100);
        if (!bgTextures.empty()) {
            currentBgIndex = bgDist(rng) % bgTextures.size();
        }

        for (int i = 0; i < 5; i++) {
            pipes.emplace_back(PIPE_SPAWN_X + i * PIPE_DISTANCE, dist(rng));
        }
    }

    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            if (!gameOver) {
                if (!gameStarted) gameStarted = true;
                bird.jump();
            }
            spacePressed = true;
        }
    } else {
        spacePressed = false;
    }
}

// Collision detection (AABB)
bool checkCollision(const Bird& b, const Pipe& p) {
    // Bird AABB
    float bLeft = b.position.x - b.size/2;
    float bRight = b.position.x + b.size/2;
    float bTop = b.position.y + b.size/2;
    float bBottom = b.position.y - b.size/2;

    // Pipe X range
    float pLeft = p.x - PIPE_WIDTH/2;
    float pRight = p.x + PIPE_WIDTH/2;

    // Check horizontal overlap
    if (bRight > pLeft && bLeft < pRight) {
        // Check vertical overlap (collision with top or bottom pipe)
        float gapTop = p.gapY + PIPE_GAP/2;
        float gapBottom = p.gapY - PIPE_GAP/2;

        if (bTop > gapTop || bBottom < gapBottom) {
            return true;
        }
    }
    return false;
}

struct GLTFMesh {
    unsigned int VAO;
    int indexCount;
    int indexType; // GL_UNSIGNED_SHORT or GL_UNSIGNED_INT
    glm::vec4 color;
};

std::vector<GLTFMesh> loadBirdModel(std::string path) {
    std::vector<GLTFMesh> meshes;
    
    std::ifstream f(path);
    if (!f) {
        std::cout << "Failed to load GLTF: " << path << std::endl;
        return meshes;
    }
    json j;
    f >> j;

    std::string binPath = path.substr(0, path.find_last_of("/\\")) + "/bird.bin";
    std::ifstream binFile(binPath, std::ios::binary);
    if (!binFile) {
        std::cout << "Failed to load binary: " << binPath << std::endl;
        return meshes;
    }
    std::vector<unsigned char> binData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());

    for (const auto& mesh : j["meshes"]) {
        if (mesh.contains("name")) {
            std::string name = mesh["name"];
            if (name == "Cube.001") continue;
        }
        for (const auto& primitive : mesh["primitives"]) {
            GLTFMesh gltfMesh;
            
            // Material
            if (primitive.contains("material")) {
                int matIdx = primitive["material"];
                auto& mat = j["materials"][matIdx];
                auto& colorFactor = mat["pbrMetallicRoughness"]["baseColorFactor"];
                gltfMesh.color = glm::vec4(colorFactor[0], colorFactor[1], colorFactor[2], colorFactor[3]);
            } else {
                gltfMesh.color = glm::vec4(1.0f);
            }

            // Indices
            int indicesIdx = primitive["indices"];
            auto& indicesAccessor = j["accessors"][indicesIdx];
            int indicesBufferViewIdx = indicesAccessor["bufferView"];
            auto& indicesBufferView = j["bufferViews"][indicesBufferViewIdx];
            int indicesOffset = (indicesAccessor.contains("byteOffset") ? (int)indicesAccessor["byteOffset"] : 0) + (int)indicesBufferView["byteOffset"];
            gltfMesh.indexCount = indicesAccessor["count"];
            int componentType = indicesAccessor["componentType"];
            gltfMesh.indexType = (componentType == 5123) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

            // Attributes
            int posIdx = primitive["attributes"]["POSITION"];
            auto& posAccessor = j["accessors"][posIdx];
            int posBufferViewIdx = posAccessor["bufferView"];
            auto& posBufferView = j["bufferViews"][posBufferViewIdx];
            int posOffset = (posAccessor.contains("byteOffset") ? (int)posAccessor["byteOffset"] : 0) + (int)posBufferView["byteOffset"];
            
            int normIdx = primitive["attributes"]["NORMAL"];
            auto& normAccessor = j["accessors"][normIdx];
            int normBufferViewIdx = normAccessor["bufferView"];
            auto& normBufferView = j["bufferViews"][normBufferViewIdx];
            int normOffset = (normAccessor.contains("byteOffset") ? (int)normAccessor["byteOffset"] : 0) + (int)normBufferView["byteOffset"];

            glGenVertexArrays(1, &gltfMesh.VAO);
            glBindVertexArray(gltfMesh.VAO);

            unsigned int posVBO;
            glGenBuffers(1, &posVBO);
            glBindBuffer(GL_ARRAY_BUFFER, posVBO);
            int posCount = posAccessor["count"];
            glBufferData(GL_ARRAY_BUFFER, posCount * 12, &binData[posOffset], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);

            unsigned int normVBO;
            glGenBuffers(1, &normVBO);
            glBindBuffer(GL_ARRAY_BUFFER, normVBO);
            int normCount = normAccessor["count"];
            glBufferData(GL_ARRAY_BUFFER, normCount * 12, &binData[normOffset], GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(1);

            unsigned int EBO;
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            int indexSize = (gltfMesh.indexType == GL_UNSIGNED_SHORT) ? 2 : 4;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, gltfMesh.indexCount * indexSize, &binData[indicesOffset], GL_STATIC_DRAW);

            meshes.push_back(gltfMesh);
        }
    }
    return meshes;
}

// Text Shader
const char* textVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)";

const char* textFragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;
    uniform sampler2D text;
    uniform vec4 textColor;
    void main() {
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = textColor * sampled;
    }
)";

stbtt_bakedchar cdata[96]; 
unsigned int fontTexture;
unsigned int textVAO, textVBO;
unsigned int textShaderProgram;

void initTextRenderer(const char* fontPath) {
    // Compile shader
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, textVertexShaderSource);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, textFragmentShaderSource);
    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vertex);
    glAttachShader(textShaderProgram, fragment);
    glLinkProgram(textShaderProgram);
    
    // Load font
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "Failed to open font file: " << fontPath << std::endl;
        return;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size)) return;

    unsigned char temp_bitmap[512*512];
    stbtt_BakeFontBitmap(buffer.data(), 0, 32.0, temp_bitmap, 512, 512, 32, 96, cdata);

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RenderText(std::string text, float x, float y, float scale, glm::vec4 color) {
    glUseProgram(textShaderProgram);
    glUniform4f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z, color.w);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindVertexArray(textVAO);

    for (char c : text) {
        if (c < 32 || c >= 128) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cdata, 512, 512, c-32, &x, &y, &q, 1);

        float vertices[6][4] = {
            { q.x0, q.y1, q.s0, q.t1 },
            { q.x1, q.y0, q.s1, q.t0 },
            { q.x0, q.y0, q.s0, q.t0 },

            { q.x0, q.y1, q.s0, q.t1 },
            { q.x1, q.y1, q.s1, q.t1 },
            { q.x1, q.y0, q.s1, q.t0 }
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

struct Button {
    float x, y, w, h;
    std::string text;
    glm::vec4 color;
    glm::vec4 hoverColor;
    
    bool isMouseOver(double mx, double my) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
};

void RenderQuad(float x, float y, float w, float h, glm::vec4 color); // Forward declaration if needed, but definition is below


void RenderQuad(float x, float y, float w, float h, glm::vec4 color) {
    glUseProgram(textShaderProgram);
    glUniform4f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z, color.w);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glBindVertexArray(textVAO);

    float vertices[6][4] = {
        { x,     y + h,   0.0f, 0.0f },
        { x + w, y,       1.0f, 1.0f },
        { x,     y,       0.0f, 1.0f },

        { x,     y + h,   0.0f, 0.0f },
        { x + w, y + h,   1.0f, 0.0f },
        { x + w, y,       1.0f, 1.0f }
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderBorder(float x, float y, float w, float h, glm::vec4 color) {
    glUseProgram(textShaderProgram);
    glUniform4f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z, color.w);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glBindVertexArray(textVAO);

    float vertices[4][4] = {
        { x,     y,       0.0f, 0.0f },
        { x + w, y,       1.0f, 0.0f },
        { x + w, y + h,   1.0f, 1.0f },
        { x,     y + h,   0.0f, 1.0f }
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glLineWidth(1.0f);
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderButton(Button& btn, double mx, double my) {
    glm::vec4 color = btn.isMouseOver(mx, my) ? btn.hoverColor : btn.color;
    RenderQuad(btn.x, btn.y, btn.w, btn.h, color);
    RenderBorder(btn.x, btn.y, btn.w, btn.h, glm::vec4(1.0f));
    
    // Approximate text centering
    float charWidth = 15.0f; // Approx width per character at scale 1.0
    float textWidth = btn.text.length() * charWidth;
    float textX = btn.x + (btn.w - textWidth) / 2.0f;
    float textY = btn.y + (btn.h / 2.0f) - 5.0f; // Vertical adjustment

    RenderText(btn.text, textX, textY, 1.0f, glm::vec4(1.0f));
}

int main() {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Flappy Bird", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Shader
    unsigned int shaderProgram = createShaderProgram();

    // Buffers
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Load Textures
    stbi_set_flip_vertically_on_load(true);
    
    // Backgrounds List - Add your new file paths here!
    std::vector<std::string> bgPaths = {
        "Resources/FlappyBird/sky/sky1.png",
        "Resources/FlappyBird/sky/sky2.jpg",
        "Resources/FlappyBird/sky/sky4.png"
    };

    for (const auto& path : bgPaths) {
        unsigned int tex = loadTexture(path.c_str());
        bgTextures.push_back(tex);
    }
    
    unsigned int pipeTexture = loadTexture("Resources/FlappyBird/pipe/Pipe.png");
    
    // Load Bird Model
    std::vector<GLTFMesh> birdMeshes = loadBirdModel("Resources/FlappyBird/bird/bird.gltf");

    // Init Text Renderer
    initTextRenderer("C:/Windows/Fonts/arial.ttf");

    // White texture for colored objects (Bird)
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char white[] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // UI Elements
    Button startBtn = {300, 250, 200, 60, "START", glm::vec4(0.2f, 0.6f, 0.2f, 0.8f), glm::vec4(0.3f, 0.8f, 0.3f, 0.9f)};
    Button restartBtn = {300, 250, 200, 60, "RESTART", glm::vec4(0.8f, 0.2f, 0.2f, 0.8f), glm::vec4(1.0f, 0.3f, 0.3f, 0.9f)};

    // Game State
    rng.seed(time(0));

    // Initialize pipes
    for (int i = 0; i < 5; i++) {
        pipes.emplace_back(PIPE_SPAWN_X + i * PIPE_DISTANCE, dist(rng));
    }

    float lastFrame = 0.0f;
    float lastBgChangeTime = 0.0f;

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Auto-change background every 10 seconds
        if (currentFrame - lastBgChangeTime >= 10.0f) {
            if (!bgTextures.empty()) {
                currentBgIndex = (currentBgIndex + 1) % bgTextures.size();
            }
            lastBgChangeTime = currentFrame;
        }

        processInput(window);

        // Update Game Logic
        if (gameStarted && !gameOver) {
            bird.update(deltaTime);

            // Move pipes
            for (auto& pipe : pipes) {
                pipe.x -= BIRD_SPEED * deltaTime;
            }

            // Recycle pipes
            if (pipes.front().x < -10.0f) {
                pipes.erase(pipes.begin());
                float lastX = pipes.back().x;
                pipes.emplace_back(lastX + PIPE_DISTANCE, dist(rng));
            }

            // Score
            for (auto& pipe : pipes) {
                if (!pipe.passed && pipe.x < bird.position.x) {
                    score++;
                    pipe.passed = true;
                }
            }

            // Collision
            for (const auto& pipe : pipes) {
                if (checkCollision(bird, pipe)) {
                    gameOver = true;
                }
            }

            // Ground/Ceiling collision
            if (bird.position.y < -5.0f || bird.position.y > 5.0f) {
                gameOver = true;
            }
        } else if (gameOver) {
             // Reset logic is now in processInput (R key) or Restart Button
        }

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Camera/View (Smooth Follow)
        static float cameraY = 0.0f;
        float targetY = bird.position.y;
        // Clamp camera so it doesn't go too wild
        if (targetY < -3.0f) targetY = -3.0f;
        if (targetY > 3.0f) targetY = 3.0f;
        
        cameraY = glm::mix(cameraY, targetY, 2.0f * deltaTime); // Smooth lerp

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, cameraY, 14.0f), 
                                     glm::vec3(0.0f, cameraY, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        unsigned int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        unsigned int texOffsetLoc = glGetUniformLocation(shaderProgram, "texOffset");
        unsigned int texScaleLoc = glGetUniformLocation(shaderProgram, "texScale");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightColorLoc, 1.0f, 0.95f, 0.9f); // Warm sunlight
        glUniform3f(lightPosLoc, 5.0f, 10.0f + cameraY, 10.0f); // Light follows camera Y
        glUniform3f(viewPosLoc, 0.0f, cameraY, 14.0f);

        glBindVertexArray(VAO);

        // Draw Background
        glBindTexture(GL_TEXTURE_2D, bgTextures[currentBgIndex]);
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        
        // Scroll background
        float bgScroll = currentFrame * 0.05f; // Slower background for parallax
        glUniform2f(texOffsetLoc, bgScroll, 0.0f);
        glUniform2f(texScaleLoc, 1.0f, 1.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, cameraY * 0.8f, -10.0f)); // Parallax Y movement
        model = glm::scale(model, glm::vec3(50.0f, 35.0f, 1.0f)); // Bigger background
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Reset texture offset for other objects
        glUniform2f(texOffsetLoc, 0.0f, 0.0f);

        // Draw Bird
        glBindTexture(GL_TEXTURE_2D, whiteTexture); // Use white texture so material colors show
        glUniform2f(texScaleLoc, 1.0f, 1.0f);
        
        for (const auto& mesh : birdMeshes) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, bird.position);
            
            // Add a slight tilt based on velocity for "aerodynamics"
            float tilt = bird.rotation;
            model = glm::rotate(model, glm::radians(tilt), glm::vec3(0.0f, 0.0f, 1.0f));
            
            // Add a slight bank when moving up/down (3D effect)
            float bank = bird.velocity * -2.0f; 
            model = glm::rotate(model, glm::radians(bank), glm::vec3(1.0f, 0.0f, 0.0f));

            // Adjust scale if needed. GLTF units are usually meters.
            // If the bird is too big/small, adjust here.
            model = glm::scale(model, glm::vec3(0.2f)); // Guessing scale
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            if (gameOver) glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
            else glUniform3f(colorLoc, mesh.color.r, mesh.color.g, mesh.color.b);
            
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0);
        }

        // Draw Pipes
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, pipeTexture);
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Use texture color
        for (const auto& pipe : pipes) {
            // Bottom pipe
            float bottomHeight = 10.0f; // Arbitrary large height
            float bottomY = pipe.gapY - PIPE_GAP/2 - bottomHeight/2;
            
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pipe.x, bottomY, 0.0f));
            model = glm::scale(model, glm::vec3(PIPE_WIDTH, bottomHeight, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform2f(texScaleLoc, 1.0f, bottomHeight * 0.5f); // Scale texture by height
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Top pipe
            float topHeight = 10.0f;
            float topY = pipe.gapY + PIPE_GAP/2 + topHeight/2;

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pipe.x, topY, 0.0f));
            model = glm::scale(model, glm::vec3(PIPE_WIDTH, topHeight, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform2f(texScaleLoc, 1.0f, topHeight * 0.5f); // Scale texture by height
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // UI Rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Mouse Input
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        static bool mousePressed = false;
        bool click = false;
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (!mousePressed) {
                click = true;
                mousePressed = true;
            }
        } else {
            mousePressed = false;
        }

        if (!gameStarted) {
            RenderText("FLAPPY BIRD 3D", 250, 400, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            RenderButton(startBtn, mx, my);
            if (click && startBtn.isMouseOver(mx, my)) {
                gameStarted = true;
                bird.jump();
            }
        } else if (gameOver) {
            RenderText("GAME OVER", 300, 350, 1.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            RenderText("Score: " + std::to_string(score), 350, 320, 1.0f, glm::vec4(1.0f));
            RenderText("Press R to Restart", 300, 250, 0.5f, glm::vec4(1.0f));
            RenderButton(restartBtn, mx, my);
            if (click && restartBtn.isMouseOver(mx, my)) {
                gameOver = false;
                gameStarted = true;
                bird.reset();
                score = 0;
                pipes.clear();
                
                // Change background randomly
                static std::uniform_int_distribution<int> bgDist(0, 100);
                if (!bgTextures.empty()) {
                    currentBgIndex = bgDist(rng) % bgTextures.size();
                }

                for (int i = 0; i < 5; i++) {
                    pipes.emplace_back(PIPE_SPAWN_X + i * PIPE_DISTANCE, dist(rng));
                }
            }
        } else {
            RenderText("Score: " + std::to_string(score), 10, 30, 1.0f, glm::vec4(1.0f));
        }
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
