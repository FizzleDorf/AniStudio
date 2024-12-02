#ifndef MESH_COMPONENT_HPP
#define MESH_COMPONENT_HPP

#include "BaseComponent.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace ECS {

struct MeshComponent : public ECS::BaseComponent {
    std::vector<glm::vec3> vertices;   // Mesh vertices
    std::vector<glm::vec3> normals;    // Mesh normals
    std::vector<glm::vec2> uvs;        // Mesh UVs
    std::vector<unsigned int> indices; // Mesh indices

    MeshComponent &operator=(const MeshComponent &other) {
        if (this != &other) {
            vertices = other.vertices;
            normals = other.normals;
            uvs = other.uvs;
            indices = other.indices;
        }
        return *this;
    }
};

} // namespace ECS

#endif // MESH_COMPONENT_HPP
