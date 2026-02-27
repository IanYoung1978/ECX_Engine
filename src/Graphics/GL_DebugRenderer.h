#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Graphics/Shader.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

class Window;

class GL_DebugRenderer
{
public:
    GL_DebugRenderer();
    ~GL_DebugRenderer();

    void init(std::shared_ptr<Window> window);
    void render(const glm::mat4& view, const glm::mat4& projection);
    void toggle() { m_Enabled = !m_Enabled; }
    bool isEnabled() const { return m_Enabled; }

private:
    void renderSphere(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);
    void renderAABB(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);
    void renderOBB(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);

    void buildSphere(float radius, int rings, int sectors);
    void buildBox(const glm::vec3& extents);
    void drawMesh(unsigned int vao, unsigned int indexCount,
        const glm::mat4& model, const glm::mat4& view,
        const glm::mat4& projection, const glm::vec4& colour);

    Shader m_WireShader;
    std::shared_ptr<Window> m_Window;
    bool m_Enabled = false;

    // Sphere wire mesh
    unsigned int m_SphereVAO = 0;
    unsigned int m_SphereVBO = 0;
    unsigned int m_SphereIBO = 0;
    unsigned int m_SphereIndexCount = 0;

    // Box wire mesh (shared for AABB and OBB, scaled by extents)
    unsigned int m_BoxVAO = 0;
    unsigned int m_BoxVBO = 0;
    unsigned int m_BoxIBO = 0;
    unsigned int m_BoxIndexCount = 0;

    static constexpr glm::vec4 COL_DEFAULT = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    static constexpr glm::vec4 COL_AABB = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
    static constexpr glm::vec4 COL_OBB = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    static constexpr glm::vec4 COL_SPHERE = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
};