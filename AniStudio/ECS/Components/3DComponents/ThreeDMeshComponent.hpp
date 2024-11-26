#ifndef THREE_D_MESH_COMPONENT_HPP
#define THREE_D_MESH_COMPONENT_HPP

#include "../ECS/Types.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace ECS {
struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;
};

struct ThreeDViewComponent : public Component {
    glm::mat4 cameraView = glm::lookAt(glm::vec3(10.f, 10.f, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 cameraProjection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 objectMatrix = glm::mat4(1.0f);

    std::vector<Mesh> meshes; // Mesh data
};
} // namespace ECS

#endif // THREE_D_MESH_COMPONENT_HPP
