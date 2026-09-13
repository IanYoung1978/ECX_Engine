#include "Procedural/EC_VolumeNode.h"
#include "Procedural/EC_Noise3D.h"
#include <algorithm>
#include <cmath>

namespace {

class ConstantNode : public EC_VolumeNode {
public:
    explicit ConstantNode(float value) : m_Value(value) {}
    float evaluate(const glm::vec3&) const override { return m_Value; }
private:
    float m_Value;
};

class NoiseNode : public EC_VolumeNode {
public:
    NoiseNode(float frequency, int octaves, float lacunarity, float persistence)
        : m_Frequency(frequency), m_Octaves(octaves), m_Lacunarity(lacunarity), m_Persistence(persistence) {}
    float evaluate(const glm::vec3& p) const override {
        return EC_Noise3D::fbm3D(p * m_Frequency, m_Octaves, m_Lacunarity, m_Persistence);
    }
private:
    float m_Frequency;
    int m_Octaves;
    float m_Lacunarity;
    float m_Persistence;
};

class SphereNode : public EC_VolumeNode {
public:
    SphereNode(const glm::vec3& center, float radius) : m_Center(center), m_Radius(radius) {}
    float evaluate(const glm::vec3& p) const override {
        return glm::length(p - m_Center) - m_Radius;
    }
private:
    glm::vec3 m_Center;
    float m_Radius;
};

class BoxNode : public EC_VolumeNode {
public:
    BoxNode(const glm::vec3& center, const glm::vec3& halfExtents) : m_Center(center), m_HalfExtents(halfExtents) {}
    float evaluate(const glm::vec3& p) const override {
        // Standard exact box SDF (Inigo Quilez): distance from the surface, negative inside.
        glm::vec3 q = glm::abs(p - m_Center) - m_HalfExtents;
        float outside = glm::length(glm::max(q, glm::vec3(0.0f)));
        float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
        return outside + inside;
    }
private:
    glm::vec3 m_Center;
    glm::vec3 m_HalfExtents;
};

class HalfspaceNode : public EC_VolumeNode {
public:
    HalfspaceNode(const glm::vec3& point, const glm::vec3& normal)
        : m_Point(point), m_Normal(glm::normalize(normal)) {}
    float evaluate(const glm::vec3& p) const override {
        return glm::dot(p - m_Point, m_Normal);
    }
private:
    glm::vec3 m_Point;
    glm::vec3 m_Normal;
};

class CylinderNode : public EC_VolumeNode {
public:
    CylinderNode(const glm::vec3& a, const glm::vec3& b, float radius) : m_A(a), m_B(b), m_Radius(radius) {}
    float evaluate(const glm::vec3& p) const override {
        // Exact capped-cylinder SDF (Inigo Quilez).
        glm::vec3 axis = m_B - m_A;
        float axisLenSq = glm::dot(axis, axis);
        if (axisLenSq < 1e-12f) return glm::length(p - m_A) - m_Radius;

        glm::vec3 pa = p - m_A;
        float t = glm::dot(pa, axis) / axisLenSq;
        float radialDist = glm::length(pa - axis * t) - m_Radius;
        float axialDist = (std::abs(t - 0.5f) - 0.5f) * std::sqrt(axisLenSq);

        float outside = std::sqrt(std::pow(std::max(radialDist, 0.0f), 2.0f) + std::pow(std::max(axialDist, 0.0f), 2.0f));
        float inside = std::min(std::max(radialDist, axialDist), 0.0f);
        return outside + inside;
    }
private:
    glm::vec3 m_A, m_B;
    float m_Radius;
};

class AddNode : public EC_VolumeNode {
public:
    AddNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b) : m_A(std::move(a)), m_B(std::move(b)) {}
    float evaluate(const glm::vec3& p) const override { return m_A->evaluate(p) + m_B->evaluate(p); }
private:
    EC_VolumeNodePtr m_A, m_B;
};

class ScaleNode : public EC_VolumeNode {
public:
    ScaleNode(EC_VolumeNodePtr a, float factor) : m_A(std::move(a)), m_Factor(factor) {}
    float evaluate(const glm::vec3& p) const override { return m_A->evaluate(p) * m_Factor; }
private:
    EC_VolumeNodePtr m_A;
    float m_Factor;
};

class UnionNode : public EC_VolumeNode {
public:
    UnionNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b) : m_A(std::move(a)), m_B(std::move(b)) {}
    float evaluate(const glm::vec3& p) const override { return std::min(m_A->evaluate(p), m_B->evaluate(p)); }
private:
    EC_VolumeNodePtr m_A, m_B;
};

class IntersectNode : public EC_VolumeNode {
public:
    IntersectNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b) : m_A(std::move(a)), m_B(std::move(b)) {}
    float evaluate(const glm::vec3& p) const override { return std::max(m_A->evaluate(p), m_B->evaluate(p)); }
private:
    EC_VolumeNodePtr m_A, m_B;
};

class SubtractNode : public EC_VolumeNode {
public:
    SubtractNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b) : m_A(std::move(a)), m_B(std::move(b)) {}
    float evaluate(const glm::vec3& p) const override { return std::max(m_A->evaluate(p), -m_B->evaluate(p)); }
private:
    EC_VolumeNodePtr m_A, m_B;
};

// Standard polynomial smooth-min (Inigo Quilez) - degenerates to std::min at k=0.
float smin(float a, float b, float k) {
    if (k <= 0.0f) return std::min(a, b);
    float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return glm::mix(b, a, h) - k * h * (1.0f - h);
}

class SmoothUnionNode : public EC_VolumeNode {
public:
    SmoothUnionNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b, float k) : m_A(std::move(a)), m_B(std::move(b)), m_K(k) {}
    float evaluate(const glm::vec3& p) const override { return smin(m_A->evaluate(p), m_B->evaluate(p), m_K); }
private:
    EC_VolumeNodePtr m_A, m_B;
    float m_K;
};

class SmoothIntersectNode : public EC_VolumeNode {
public:
    SmoothIntersectNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b, float k) : m_A(std::move(a)), m_B(std::move(b)), m_K(k) {}
    float evaluate(const glm::vec3& p) const override { return -smin(-m_A->evaluate(p), -m_B->evaluate(p), m_K); }
private:
    EC_VolumeNodePtr m_A, m_B;
    float m_K;
};

class SmoothSubtractNode : public EC_VolumeNode {
public:
    SmoothSubtractNode(EC_VolumeNodePtr a, EC_VolumeNodePtr b, float k) : m_A(std::move(a)), m_B(std::move(b)), m_K(k) {}
    float evaluate(const glm::vec3& p) const override { return -smin(-m_A->evaluate(p), m_B->evaluate(p), m_K); }
private:
    EC_VolumeNodePtr m_A, m_B;
    float m_K;
};

class TranslateNode : public EC_VolumeNode {
public:
    TranslateNode(EC_VolumeNodePtr a, const glm::vec3& offset) : m_A(std::move(a)), m_Offset(offset) {}
    float evaluate(const glm::vec3& p) const override { return m_A->evaluate(p - m_Offset); }
private:
    EC_VolumeNodePtr m_A;
    glm::vec3 m_Offset;
};

} // namespace

namespace EC_Volume {

EC_VolumeNodePtr constant(float value) { return std::make_shared<ConstantNode>(value); }

EC_VolumeNodePtr noise(float frequency, int octaves, float lacunarity, float persistence) {
    return std::make_shared<NoiseNode>(frequency, octaves, lacunarity, persistence);
}

EC_VolumeNodePtr sphere(const glm::vec3& center, float radius) { return std::make_shared<SphereNode>(center, radius); }
EC_VolumeNodePtr box(const glm::vec3& center, const glm::vec3& halfExtents) { return std::make_shared<BoxNode>(center, halfExtents); }
EC_VolumeNodePtr halfspace(const glm::vec3& point, const glm::vec3& normal) { return std::make_shared<HalfspaceNode>(point, normal); }
EC_VolumeNodePtr cylinder(const glm::vec3& pointA, const glm::vec3& pointB, float radius) { return std::make_shared<CylinderNode>(pointA, pointB, radius); }

EC_VolumeNodePtr add(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b) { return std::make_shared<AddNode>(a, b); }
EC_VolumeNodePtr scale(const EC_VolumeNodePtr& a, float factor) { return std::make_shared<ScaleNode>(a, factor); }

EC_VolumeNodePtr unionOf(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b) { return std::make_shared<UnionNode>(a, b); }
EC_VolumeNodePtr intersect(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b) { return std::make_shared<IntersectNode>(a, b); }
EC_VolumeNodePtr subtract(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b) { return std::make_shared<SubtractNode>(a, b); }

EC_VolumeNodePtr smoothUnion(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k) { return std::make_shared<SmoothUnionNode>(a, b, k); }
EC_VolumeNodePtr smoothIntersect(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k) { return std::make_shared<SmoothIntersectNode>(a, b, k); }
EC_VolumeNodePtr smoothSubtract(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k) { return std::make_shared<SmoothSubtractNode>(a, b, k); }

EC_VolumeNodePtr translate(const EC_VolumeNodePtr& a, const glm::vec3& offset) { return std::make_shared<TranslateNode>(a, offset); }

} // namespace EC_Volume
