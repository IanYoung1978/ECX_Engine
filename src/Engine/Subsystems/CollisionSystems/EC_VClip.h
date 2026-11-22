#pragma once
#include "EC_CollisionShapes.h"
#include <glm/glm.hpp>

// Box-specialised V-Clip: given two overlapping OBBs plus OBBVsOBB's normal/depth, finds the true
// closest feature pair by local Voronoi classification and generates the contact points. Replaces
// SAT's numeric overlap comparison, which misclassifies edge-edge for very different box sizes.
namespace EC_VClip
{
    enum class FeatureType { Vertex, Edge, Face };

    // index meaning depends on type: vertex 0-7, edge 0-11, face 0-5 - see the
    // topology tables in EC_VClip.cpp for exactly what each index represents.
    struct Feature
    {
        FeatureType type = FeatureType::Face;
        int index = 0;
    };

    // Walks from the SAT normal (only an initial seed; the walk corrects it) to the true closest
    // feature pair and fills manifold.contactPoints (1 point for a vertex/edge pair, up to 4 for a
    // face-face rest via Sutherland-Hodgman clipping). contactNormal/penetrationDepth stay as SAT
    // computed them.
    void generateContactPoints(
        const OBB& obbA, const glm::vec3& posA,
        const OBB& obbB, const glm::vec3& posB,
        const glm::vec3& normal, float penetrationDepth,
        CollisionManifold& manifold);
}
