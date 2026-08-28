#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Graphics/Shader.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "Graphics/DebugVisualization.h"

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
    // True if render() would actually draw anything - collider wireframes are toggled,
    // but a shown ray/cone should still render even while that toggle is off.
    bool hasVisualization() const { return m_Enabled || m_HasRay || m_HasCone; }

    // Issue #30/#29 verification: visualize the last ray/cone query fired from script,
    // drawn every frame regardless of the collider-wireframe toggle above until replaced
    // by a newer call - independent, on-demand debug drawing, not lumped with m_Enabled.
    void showRay(const DebugRayVisualization& ray) { m_Ray = ray; m_HasRay = true; }
    void showCone(const DebugConeVisualization& cone) { m_Cone = cone; m_HasCone = true; }

private:
    void renderSphere(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);
    void renderAABB(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);
    void renderOBB(const glm::mat4& view, const glm::mat4& projection,
        const EC_DOD_Collider& collider, const EC_DOD_Transform& transform);
    // Draws a small 3-axis cross marker at every contact point of every
    // currently-colliding entity, read via EC_DOD_DebugContacts (published
    // once per physics tick by EC_PhysicsSystem) - the same manifold points
    // EC_PhysicsResolution actually resolves against, so this is a direct
    // visual check of what the narrow phase is generating (point count,
    // position, clustering) rather than just the coarse collider wireframes
    // above. Tied to the same m_Enabled toggle. Deliberately does not read
    // EC_PairManager directly - that's physics-thread-internal state; this
    // component is the sanctioned cross-thread hand-off.
    void renderContactPoints(const glm::mat4& view, const glm::mat4& projection);
    void buildCrossLines(const glm::vec3& point, float halfSize, std::vector<glm::vec3>& outLines) const;

    void buildSphere(float radius, int rings, int sectors);
    void buildBox(const glm::vec3& extents);
    void drawMesh(unsigned int vao, unsigned int indexCount,
        const glm::mat4& model, const glm::mat4& view,
        const glm::mat4& projection, const glm::vec4& colour);

    // Ad-hoc line drawing for the ray/cone visualization - a dynamic (not indexed,
    // not static-built) VBO rebuilt each draw call, unlike the sphere/box meshes above.
    void drawLines(const std::vector<glm::vec3>& points, const glm::vec4& colour,
        const glm::mat4& view, const glm::mat4& projection);
    // Wireframe cone as a base ring + spokes from the apex, built fresh each call from
    // an orthonormal basis derived from `dir` (mirrors the direction/right/up derivation
    // EC_SpatialSystem::update() already uses).
    void buildConeLines(const glm::vec3& apex, const glm::vec3& dir, float halfAngleRadians,
        float length, std::vector<glm::vec3>& outLines) const;

    Shader m_WireShader;
    std::shared_ptr<Window> m_Window;
    bool m_Enabled = false;

    unsigned int m_LineVAO = 0;
    unsigned int m_LineVBO = 0;

    bool m_HasRay = false;
    DebugRayVisualization m_Ray;
    bool m_HasCone = false;
    DebugConeVisualization m_Cone;

    static constexpr glm::vec4 COL_RAY = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    static constexpr glm::vec4 COL_CONE = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);

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
    static constexpr glm::vec4 COL_CONTACT = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
};