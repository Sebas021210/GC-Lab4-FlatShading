#pragma once
#include "glm/glm.hpp"
#include "Color.h"

struct Vertex {
    glm::vec3 position;
    Color color;
};

struct Fragment {
    glm::vec3 position;
    Color color;
    glm::vec3 normal; // Normal del fragmento
};
