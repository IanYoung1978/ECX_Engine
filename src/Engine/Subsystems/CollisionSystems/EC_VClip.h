#pragma once
#include "EC_CollisionShapes.h"
#include <glm/glm.hpp>

// Box-specialised V-Clip: given two OBBs already known to overlap (from
// OBBVsOBB's 15-axis SAT test) plus that test's normal/depth, walks to the
// TRUE closest feature pair (vertex/edge/face on each box) via local-frame
// Voronoi-region classification, then generates the resulting contact
// point(s). This replaces choosing the manifold shape from SAT's raw numeric
// overlap comparison, which misclassifies "edge-edge" for grossly mismatched
// body sizes (e.g. a cube vs a room-sized floor slab) - the numeric overlap
// values can be close even when the TRUE closest feature is obviously the
// floor's face, and a contact point computed from the floor's actual
// geometric edges (sitting at the slab's far perimeter) has nothing to do
// with where the cube is really touching down.
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

    // Walks from the SAT-provided normal (used only as an initial seed point -
    // the walk corrects it if wrong) to the true closest feature pair, then
    // fills manifold.contactPoints (1 point for any vertex/edge-involving pair,
    // up to 4 for a genuine face-face rest, via the same Sutherland-Hodgman
    // clip used before). Does not touch manifold.contactNormal/
    // penetrationDepth - those stay authoritative from the SAT stage, which is
    // already correct and robust; this function's only job is picking the
    // right feature pair and contact location.
    void generateContactPoints(
        const OBB& obbA, const glm::vec3& posA,
        const OBB& obbB, const glm::vec3& posB,
        const glm::vec3& normal, float penetrationDepth,
        CollisionManifold& manifold);
}
