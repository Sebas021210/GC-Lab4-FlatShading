#pragma once
#include "color.h"
#include "Uniforms.h"
#include "fragment.h"
#include <cmath>
#include <random>

Vertex vertexShader(const Vertex& vertex, const Uniforms& u) {
    glm::vec4 v = glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1);
    glm::vec4 r = u.viewport * u.projection * u.view * u.model * v;

    return Vertex{
            glm::vec3(r.x/r.w, r.y/r.w, r.z/r.w),
            vertex.color
    };
};

Fragment fragmentShader(Fragment fragment) {
    return fragment;
};
