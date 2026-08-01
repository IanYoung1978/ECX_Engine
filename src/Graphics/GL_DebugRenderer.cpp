#include "Graphics/GL_DebugRenderer.h"
#include "Window/Window.h"
#include "Logging/ECX_Logging.h"
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <cmath>

GL_DebugRenderer::GL_DebugRenderer() {}

GL_DebugRenderer::~GL_DebugRenderer()
{
    if (m_SphereVAO) glDeleteVertexArrays(1, &m_SphereVAO);
    if (m_SphereVBO) glDeleteBuffers(1, &m_SphereVBO);
    if (m_SphereIBO) glDeleteBuffers(1, &m_SphereIBO);
    if (m_BoxVAO)    glDeleteVertexArrays(1, &m_BoxVAO);
    if (m_BoxVBO)    glDeleteBuffers(1, &m_BoxVBO);
    if (m_BoxIBO)    glDeleteBuffers(1, &m_BoxIBO);
    if (m_LineVAO)   glDeleteVertexArrays(1, &m_LineVAO);
    if (m_LineVBO)   glDeleteBuffers(1, &m_LineVBO);
}

void GL_DebugRenderer::init(std::shared_ptr<Window> window)
{
    m_Window = window;

    if (!m_WireShader.loadShader(
        "data/assets/shaders/wireframe.vert",
        "data/assets/shaders/wireframe.frag"))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load wireframe shader",
            LOGGING::LogLevel::CRITICAL);
        return;
    }

    buildSphere(1.0f, 16, 16);
    buildBox(glm::vec3(1.0f));

    glGenVertexArrays(1, &m_LineVAO);
    glBindVertexArray(m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL_DebugRenderer::buildSphere(float radius, int rings, int sectors)
{
    std::vector<glm::vec3> verts;
    std::vector<unsigned int> indices;

    for (int r = 0; r <= rings; r++) {
        float phi = glm::pi<float>() * r / rings;
        for (int s = 0; s <= sectors; s++) {
            float theta = 2.0f * glm::pi<float>() * s / sectors;
            float x = radius * std::sin(phi) * std::cos(theta);
            float y = radius * std::cos(phi);
            float z = radius * std::sin(phi) * std::sin(theta);
            verts.push_back(glm::vec3(x, y, z));
        }
    }

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            int cur = r * (sectors + 1) + s;
            int next = cur + sectors + 1;
            // horizontal ring line
            indices.push_back(cur);
            indices.push_back(cur + 1);
            // vertical meridian line
            indices.push_back(cur);
            indices.push_back(next);
        }
    }

    m_SphereIndexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &m_SphereVAO);
    glBindVertexArray(m_SphereVAO);

    glGenBuffers(1, &m_SphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_SphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_SphereIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GL_DebugRenderer::buildBox(const glm::vec3& extents)
{
    // Unit box, scaled at draw time via model matrix
    float x = extents.x;
    float y = extents.y;
    float z = extents.z;

    std::vector<glm::vec3> verts = {
        { -x, -y, -z }, {  x, -y, -z },
        {  x,  y, -z }, { -x,  y, -z },
        { -x, -y,  z }, {  x, -y,  z },
        {  x,  y,  z }, { -x,  y,  z }
    };

    std::vector<unsigned int> indices = {
        // bottom
        0, 1,  1, 2,  2, 3,  3, 0,
        // top
        4, 5,  5, 6,  6, 7,  7, 4,
        // sides
        0, 4,  1, 5,  2, 6,  3, 7
    };

    m_BoxIndexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &m_BoxVAO);
    glBindVertexArray(m_BoxVAO);

    glGenBuffers(1, &m_BoxVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_BoxVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_BoxIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BoxIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GL_DebugRenderer::drawMesh(unsigned int vao, unsigned int indexCount,
    const glm::mat4& model, const glm::mat4& view,
    const glm::mat4& projection, const glm::vec4& colour)
{
    m_WireShader.setUniform("ModelTransform", model);
    m_WireShader.setUniform("ViewTransform", view);
    m_WireShader.setUniform("ProjTransform", projection);
    m_WireShader.setUniform("wireColour", colour);

    glBindVertexArray(vao);
    glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GL_DebugRenderer::renderSphere(const glm::mat4& view, const glm::mat4& projection,
    const EC_DOD_Collider& collider, const EC_DOD_Transform& transform)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(transform.matrix[3]));
    glm::mat4 rotation = transform.matrix;
    rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    model = model * rotation;
    model = glm::scale(model, glm::vec3(collider.radius * 1.01f)); // 1% larger
    drawMesh(m_SphereVAO, m_SphereIndexCount, model, view, projection, COL_SPHERE);
}

void GL_DebugRenderer::renderAABB(const glm::mat4& view, const glm::mat4& projection,
    const EC_DOD_Collider& collider, const EC_DOD_Transform& transform)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(transform.matrix[3]));
    model = glm::scale(model, collider.extents * 1.01f); // 1% larger
    drawMesh(m_BoxVAO, m_BoxIndexCount, model, view, projection, COL_AABB);
}

void GL_DebugRenderer::renderOBB(const glm::mat4& view, const glm::mat4& projection,
    const EC_DOD_Collider& collider, const EC_DOD_Transform& transform)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(transform.matrix[3]));
    glm::mat4 rotation = transform.matrix;
    rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    model = model * rotation;
    model = glm::scale(model, collider.extents * 1.01f); // 1% larger
    drawMesh(m_BoxVAO, m_BoxIndexCount, model, view, projection, COL_OBB);
}

void GL_DebugRenderer::drawLines(const std::vector<glm::vec3>& points, const glm::vec4& colour,
    const glm::mat4& view, const glm::mat4& projection)
{
    if (points.empty()) return;

    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);

    m_WireShader.setUniform("ModelTransform", glm::mat4(1.0f));
    m_WireShader.setUniform("ViewTransform", view);
    m_WireShader.setUniform("ProjTransform", projection);
    m_WireShader.setUniform("wireColour", colour);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

void GL_DebugRenderer::buildConeLines(const glm::vec3& apex, const glm::vec3& dir, float halfAngleRadians,
    float length, std::vector<glm::vec3>& outLines) const
{
    glm::vec3 forward = glm::normalize(dir);
    glm::vec3 worldUp = (std::abs(forward.y) > 0.99f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    float ringRadius = length * std::tan(halfAngleRadians);
    glm::vec3 ringCenter = apex + forward * length;

    const int segments = 16;
    std::vector<glm::vec3> ring(segments);
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / segments;
        ring[i] = ringCenter + right * (ringRadius * std::cos(theta)) + up * (ringRadius * std::sin(theta));
    }

    for (int i = 0; i < segments; i++) {
        outLines.push_back(ring[i]);
        outLines.push_back(ring[(i + 1) % segments]);
    }
    for (int i = 0; i < segments; i += 2) {
        outLines.push_back(apex);
        outLines.push_back(ring[i]);
    }
}

void GL_DebugRenderer::buildCrossLines(const glm::vec3& point, float halfSize, std::vector<glm::vec3>& outLines) const
{
    outLines.push_back(point - glm::vec3(halfSize, 0.0f, 0.0f));
    outLines.push_back(point + glm::vec3(halfSize, 0.0f, 0.0f));
    outLines.push_back(point - glm::vec3(0.0f, halfSize, 0.0f));
    outLines.push_back(point + glm::vec3(0.0f, halfSize, 0.0f));
    outLines.push_back(point - glm::vec3(0.0f, 0.0f, halfSize));
    outLines.push_back(point + glm::vec3(0.0f, 0.0f, halfSize));
}

void GL_DebugRenderer::renderContactPoints(const glm::mat4& view, const glm::mat4& projection)
{
    constexpr float kCrossHalfSize = 0.1f;

    std::vector<glm::vec3> lines;
    for (const EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
        if (!pair.m_Colliding) continue;
        for (const glm::vec3& point : pair.m_CollisionPoints) {
            buildCrossLines(point, kCrossHalfSize, lines);
        }
    }
    drawLines(lines, COL_CONTACT, view, projection);
}

void GL_DebugRenderer::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_Enabled && !m_HasRay && !m_HasCone) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glLineWidth(3.0f);

    m_WireShader.activate();

    if (m_Enabled) {
        auto& manager = EC_DOD_EntityManager::getInstance();
        auto entities = manager.getEntitiesWithComponents({
            std::type_index(typeid(EC_DOD_Collider)),
            std::type_index(typeid(EC_DOD_Transform))
            });

        for (EntityID entity : entities) {
            if (!manager.isAlive(entity)) continue;

            const auto& collider = manager.getComponent<EC_DOD_Collider>(entity);
            const auto& transform = manager.getComponent<EC_DOD_Transform>(entity);

            switch (collider.type) {
            case EC_DOD_Collider::Type::Sphere:
                renderSphere(view, projection, collider, transform);
                break;
            case EC_DOD_Collider::Type::AABB:
                renderAABB(view, projection, collider, transform);
                break;
            case EC_DOD_Collider::Type::OBB:
                renderOBB(view, projection, collider, transform);
                break;
            default:
                break;
            }
        }

        renderContactPoints(view, projection);
    }

    if (m_HasRay) {
        std::vector<glm::vec3> pts{ m_Ray.origin, m_Ray.origin + m_Ray.direction * m_Ray.maxDistance };
        drawLines(pts, COL_RAY, view, projection);
    }

    if (m_HasCone) {
        std::vector<glm::vec3> pts;
        buildConeLines(m_Cone.apex, m_Cone.direction, m_Cone.halfAngleRadians, m_Cone.maxDistance, pts);
        drawLines(pts, COL_CONE, view, projection);
    }

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glUseProgram(0);
}