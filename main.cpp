#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Vertices.h"
#include "Fragment.h"
#include "Uniforms.h"
#include "Color.h"
#include "Triangle.h"
#include "Shaders.h"
#include "Print.h"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT> zbuffer;
Uniforms uniform;

// Declare a global clearColor of type Color
Color clearColor = {0, 0, 0}; // Initially set to black

// Declare a global currentColor of type Color
Color currentColor = {255, 255, 255}; // Initially set to white

void init() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

void clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (auto &row : zbuffer) {
        std::fill(row.begin(), row.end(), 99999.0f);
    }
}

struct Face {
    std::vector<std::array<int, 3>> vertexIndices;
    std::vector<int> uvIndices;
    std::vector<int> normalIndices;
};

struct Camera {
    glm::vec3 cameraPosition;
    glm::vec3 targetPosition;
    glm::vec3 upVector;
};

// Estructura para la cabecera de archivo BMP
#pragma pack(push, 1)
struct BMPFileHeader {
    char header[2] = {'B', 'M'};
    uint32_t file_size;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;
    uint32_t data_offset;
};

// Estructura para la cabecera de información BMP
struct BMPInfoHeader {
    uint32_t size = 40;
    int32_t width;
    int32_t height;
    uint16_t planes = 1;
    uint16_t bpp = 32;  // 32 bits por píxel para un z-buffer
    uint32_t compression = 0;
    uint32_t image_size = 0;
    int32_t x_pixels_per_meter = 0;
    int32_t y_pixels_per_meter = 0;
    uint32_t total_colors = 0;
    uint32_t important_colors = 0;
};
#pragma pack(pop)

std::vector<glm::vec3> vertices;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;
std::vector<Face> faces;

bool loadOBJ(const std::string& path) {
    std::vector<glm::vec3> out_vertices; // Vector para almacenar los vértices
    std::vector<Face> out_faces; // Vector para almacenar las caras

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex); // Agrega el vértice al vector global de vértices
        } else if (type == "f") {
            std::string lineHeader;
            Face face;
            while (iss >> lineHeader) {
                std::istringstream tokenstream(lineHeader);
                std::string token;
                std::array<int, 3> vertexIndices;

                // Lee los tres valores separados por '/' en cada vértice de la cara
                for (int i = 0; i < 3; ++i) {
                    std::getline(tokenstream, token, '/');
                    vertexIndices[i] = std::stoi(token) - 1;
                }

                face.vertexIndices.push_back(vertexIndices); // Agrega los índices de vértices al objeto Face
            }
            faces.push_back(face); // Agrega la cara al vector global de caras
        }
    }

    file.close();
    return true;
}

std::vector<Vertex> setupVertex(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces){
    std::vector<Vertex> vertexArray;

    // Reserve espacio para todos los vértices para evitar realocaciones frecuentes
    vertexArray.reserve(faces.size() * 3);

    // Itera sobre las caras y sus vértices
    for (const auto& face : faces) {
        for (const auto& vertexIndices : face.vertexIndices) {
            glm::vec3 vertexPosition = vertices[vertexIndices[0]];
            Vertex vertex;
            vertex.position = vertexPosition;

            // Establece el color
            vertex.color = {255, 255, 255};

            // Agrega el vértice al vector de vértices
            vertexArray.push_back(vertex);
        }
    }

    return vertexArray;
}

// Function to set a specific pixel in the framebuffer to the currentColor
void point(Fragment f) {
    if (f.position.z < zbuffer[f.position.y][f.position.x]) {
        SDL_SetRenderDrawColor(renderer, f.color.r, f.color.g, f.color.b, f.color.a);
        SDL_RenderDrawPoint(renderer, f.position.x, f.position.y);
        zbuffer[f.position.y][f.position.x] = f.position.z;
    }
}

std::vector<std::vector<Vertex>> primitiveAssembly(const std::vector<Vertex>& transformedVertices) {
    std::vector<std::vector<Vertex>> groupedVertices;

    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);
        groupedVertices.push_back(vertexGroup);
    }

    return groupedVertices;
}

void render(const std::vector<Vertex>& vertices) {
    // 1. Vertex Shader
    // vertex -> transformedVertices
    std::vector<Vertex> transformedVertices;
    for (const Vertex& vertex : vertices) {
        Vertex transformedVertex = vertexShader(vertex, uniform);
        transformedVertices.push_back(transformedVertex);
    }

    // 2. Primitive Assembly
    // transformedVertices -> triangles
    std::vector<std::vector<Vertex>> triangles = primitiveAssembly(transformedVertices);

    // 3. Rasterize
    // triangles -> Fragments
    std::vector<Fragment> fragments;
    for (const std::vector<Vertex>& triangleVertices : triangles) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                triangleVertices[0],
                triangleVertices[1],
                triangleVertices[2]
        );

        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    // 4. Fragment Shader
    // Fragments -> colors
    for (Fragment fragment : fragments) {
        point(fragmentShader(fragment));
    }
}

float rotationAngle = 3.14f / 3.0f;
glm::mat4 createModelMatrix() {
    rotationAngle += 0.02f; // Incrementa el ángulo de rotación
    glm::mat4 rotationY = glm::rotate(glm::mat4(1), rotationAngle, glm::vec3(0, 1, 0));
    glm::mat4 rotationX = glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(1, 0, 0));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.20f));

    return rotationX * rotationY * scale;
}

glm::mat4 createViewMatrix() {
    return glm::lookAt(
            // donde esta
            glm::vec3(0, 0, -5),
            // hacia adonde mira
            glm::vec3(0, 0, 0),
            // arriba
            glm::vec3(0, 1, 0)
    );
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = SCREEN_WIDTH / SCREEN_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

// Función para guardar el z-buffer en un archivo BMP
void saveZBufferToBMP(const std::array<std::array<float, SCREEN_WIDTH>, SCREEN_HEIGHT>& zbuffer) {
    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    file_header.file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + (SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    file_header.data_offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    info_header.width = SCREEN_WIDTH;
    info_header.height = SCREEN_HEIGHT;

    std::ofstream bmp_file("zbuffer.bmp", std::ios::out | std::ios::binary);
    if (!bmp_file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo BMP para escritura." << std::endl;
        return;
    }

    // Escribe las cabeceras en el archivo BMP
    bmp_file.write(reinterpret_cast<char*>(&file_header), sizeof(BMPFileHeader));
    bmp_file.write(reinterpret_cast<char*>(&info_header), sizeof(BMPInfoHeader));

    // Escribe los datos del z-buffer en el archivo BMP
    for (int y = SCREEN_HEIGHT - 1; y >= 0; --y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            uint32_t zvalue = static_cast<uint32_t>(zbuffer[y][x] * 255.0f);
            bmp_file.write(reinterpret_cast<char*>(&zvalue), 4);
        }
    }

    bmp_file.close();
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("Flat Shading", 380, 180, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

    // Carga el modelo OBJ
    if (!loadOBJ("../OBJ/nave.obj")) {
        std::cerr << "Error al cargar el modelo OBJ." << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Crea el vertex array
    std::vector<Vertex> vertices = setupVertex(::vertices, faces);

    // Bucle principal
    bool running = true;
    SDL_Event event;

    std::vector<glm::vec3> vertexBufferObject = {
            {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
            {-0.87f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.87f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},

            {0.0f, 1.0f,    -1.0f}, {1.0f, 1.0f, 0.0f},
            {-0.87f, -0.5f, -1.0f}, {0.0f, 1.0f, 1.0f},
            {0.87f,  -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}
    };

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event. type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        break;
                    case SDLK_DOWN:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_s:
                        break;
                    case SDLK_a:
                        break;
                }
            }
        }

        uniform.model = createModelMatrix();
        uniform.view = createViewMatrix();
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        clear();

        // Call our render function
        render(vertices);

        // Present the frame buffer to the screen
        SDL_RenderPresent(renderer);

        // Delay to limit the frame rate
        SDL_Delay(1000 / 60);

        // Llama a la función saveZBufferToBMP para guardar el z-buffer como un archivo BMP
        saveZBufferToBMP(zbuffer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
