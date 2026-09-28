#include "EC_VClip.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kEpsilon = 1e-5f;
    // Tolerance for feature classification and face clipping, much looser than kEpsilon: a resting
    // box drifts by more than 1e-5 per tick, and anything tighter flickers the manifold's point count.
    constexpr float kFeatureEpsilon = 1e-3f;

    // 8 local-space vertices, indexed by a sign triple packed into 3 bits
    // (bit0 = X sign, bit1 = Y sign, bit2 = Z sign; 1 = +halfExtent, 0 = -halfExtent).
    glm::vec3 vertexLocal(int idx, const glm::vec3& he)
    {
        return glm::vec3(
            (idx & 1) ? he.x : -he.x,
            (idx & 2) ? he.y : -he.y,
            (idx & 4) ? he.z : -he.z);
    }

    // 12 edges as vertex-index pairs - each pair differs in exactly one bit
    // (the axis the edge runs along). Grouped by that axis: vary X, vary Y, vary Z.
    constexpr int kEdgeVertices[12][2] = {
        {0,1},{2,3},{4,5},{6,7},   // vary X
        {0,2},{1,3},{4,6},{5,7},   // vary Y
        {0,4},{1,5},{2,6},{3,7},   // vary Z
    };

    // Face index = axis*2 + (sign>0 ? 1 : 0).
    int faceAxisOf(int f) { return f / 2; }
    float faceSignOf(int f) { return (f % 2) ? 1.0f : -1.0f; }

    int edgeVaryAxis(int e)
    {
        int v0 = kEdgeVertices[e][0], v1 = kEdgeVertices[e][1];
        int diff = v0 ^ v1;
        return (diff == 1) ? 0 : (diff == 2) ? 1 : 2;
    }

    // Closest point on/in a box to local point q, plus its feature (vertex/edge/face). For an
    // axis-aligned local box, the number of axes that needed clamping identifies the feature (1 face,
    // 2 edge, 3 vertex). If q is inside, use the axis with the least penetration margin.
    EC_VClip::Feature classifyLocalPoint(const glm::vec3& q, const glm::vec3& he, glm::vec3& outClosestLocal)
    {
        bool clamped[3] = { false, false, false };
        float sign[3] = { 0.0f, 0.0f, 0.0f };
        int numClamped = 0;
        glm::vec3 closest = q;

        for (int axis = 0; axis < 3; axis++)
        {
            if (q[axis] > he[axis] - kFeatureEpsilon)
            {
                clamped[axis] = true; sign[axis] = 1.0f; closest[axis] = he[axis]; numClamped++;
            }
            else if (q[axis] < -he[axis] + kFeatureEpsilon)
            {
                clamped[axis] = true; sign[axis] = -1.0f; closest[axis] = -he[axis]; numClamped++;
            }
        }

        if (numClamped == 0)
        {
            int bestAxis = 0;
            float bestMargin = he.x - std::abs(q.x);
            for (int axis = 1; axis < 3; axis++)
            {
                float margin = he[axis] - std::abs(q[axis]);
                if (margin < bestMargin) { bestMargin = margin; bestAxis = axis; }
            }
            clamped[bestAxis] = true;
            sign[bestAxis] = (q[bestAxis] >= 0.0f) ? 1.0f : -1.0f;
            closest[bestAxis] = sign[bestAxis] * he[bestAxis];
            numClamped = 1;
        }

        outClosestLocal = closest;

        EC_VClip::Feature feature;
        if (numClamped == 1)
        {
            int axis = clamped[0] ? 0 : (clamped[1] ? 1 : 2);
            feature.type = EC_VClip::FeatureType::Face;
            feature.index = axis * 2 + (sign[axis] > 0.0f ? 1 : 0);
        }
        else if (numClamped == 2)
        {
            int varyAxis = clamped[0] ? (clamped[1] ? 2 : 1) : 0;
            feature.type = EC_VClip::FeatureType::Edge;
            feature.index = 0;
            for (int e = 0; e < 12; e++)
            {
                if (edgeVaryAxis(e) != varyAxis) continue;
                int v0 = kEdgeVertices[e][0];
                bool match = true;
                for (int axis = 0; axis < 3; axis++)
                {
                    if (axis == varyAxis || !clamped[axis]) continue;
                    float vSign = (v0 & (1 << axis)) ? 1.0f : -1.0f;
                    if (vSign != sign[axis]) { match = false; break; }
                }
                if (match) { feature.index = e; break; }
            }
        }
        else
        {
            int idx = 0;
            for (int axis = 0; axis < 3; axis++) if (sign[axis] > 0.0f) idx |= (1 << axis);
            feature.type = EC_VClip::FeatureType::Vertex;
            feature.index = idx;
        }
        return feature;
    }

    std::vector<glm::vec3> featureWorldPoints(const EC_VClip::Feature& f, const glm::vec3& center,
        const glm::mat3& orient, const glm::vec3& he)
    {
        if (f.type == EC_VClip::FeatureType::Vertex)
            return { center + orient * vertexLocal(f.index, he) };

        if (f.type == EC_VClip::FeatureType::Edge)
        {
            glm::vec3 v0 = vertexLocal(kEdgeVertices[f.index][0], he);
            glm::vec3 v1 = vertexLocal(kEdgeVertices[f.index][1], he);
            return { center + orient * v0, center + orient * v1 };
        }

        return {};
    }

    // Keeps only points that project, in the face-box's local frame, within
    // the face's rectangular lateral extent (the two axes other than the
    // face's own normal axis) - i.e. points genuinely over the face, not off
    // one of its edges.
    std::vector<glm::vec3> clipPointsAgainstFace(const std::vector<glm::vec3>& worldPoints,
        const glm::vec3& faceBoxCenter, const glm::mat3& faceBoxOrient, const glm::vec3& faceBoxHalfExtents,
        int faceIndex)
    {
        int faceAx = faceAxisOf(faceIndex);
        int u = (faceAx + 1) % 3, v = (faceAx + 2) % 3;
        glm::mat3 invOrient = glm::transpose(faceBoxOrient);

        std::vector<glm::vec3> result;
        for (const glm::vec3& p : worldPoints)
        {
            glm::vec3 local = invOrient * (p - faceBoxCenter);
            if (std::abs(local[u]) <= faceBoxHalfExtents[u] + kFeatureEpsilon &&
                std::abs(local[v]) <= faceBoxHalfExtents[v] + kFeatureEpsilon)
            {
                result.push_back(p);
            }
        }
        return result;
    }

    // The 4 world-space corners of one face of a box - identical to the
    // helper formerly local to EC_CollisionChecks.cpp's OBBVsOBB; moved here
    // since face-face manifold generation now lives in this module.
    std::vector<glm::vec3> boxFaceCorners(const glm::vec3& center, const glm::vec3& he,
        const glm::mat3& orient, int axis, float sign)
    {
        int u = (axis + 1) % 3, v = (axis + 2) % 3;
        glm::vec3 faceLocalCenter(0.0f);
        faceLocalCenter[axis] = sign * he[axis];

        std::vector<glm::vec3> corners;
        corners.reserve(4);
        for (float su : { -1.0f, 1.0f })
        {
            for (float sv : { -1.0f, 1.0f })
            {
                glm::vec3 local = faceLocalCenter;
                local[u] = su * he[u];
                local[v] = sv * he[v];
                corners.push_back(center + orient * local);
            }
        }
        // Reorder to a consistent winding (su,sv) = (-,-),(+,-),(+,+),(-,+)
        std::swap(corners[2], corners[3]);
        return corners;
    }

    std::vector<glm::vec3> clipPolygonAgainstPlane(const std::vector<glm::vec3>& poly,
        const glm::vec3& planePoint, const glm::vec3& planeNormal)
    {
        if (poly.size() < 2) return poly;

        std::vector<glm::vec3> result;
        result.reserve(poly.size() + 1);

        for (size_t i = 0; i < poly.size(); i++)
        {
            const glm::vec3& current = poly[i];
            const glm::vec3& next = poly[(i + 1) % poly.size()];

            float distCurrent = glm::dot(current - planePoint, planeNormal);
            float distNext = glm::dot(next - planePoint, planeNormal);

            bool currentInside = distCurrent <= kEpsilon;
            bool nextInside = distNext <= kEpsilon;

            if (currentInside) result.push_back(current);

            if (currentInside != nextInside)
            {
                float denom = distCurrent - distNext;
                if (std::abs(denom) > kEpsilon)
                {
                    float t = distCurrent / denom;
                    result.push_back(current + t * (next - current));
                }
            }
        }
        return result;
    }
}

namespace EC_VClip
{
    void generateContactPoints(
        const OBB& obbA, const glm::vec3& posA,
        const OBB& obbB, const glm::vec3& posB,
        const glm::vec3& normal, float /*penetrationDepth*/,
        CollisionManifold& manifold)
    {
        const glm::vec3 centerA = posA + obbA.center;
        const glm::vec3 centerB = posB + obbB.center;

        // Seed the walk at the closest point on box A to box B's centre (B's centre in A's frame, clamped),
        // not offset along the SAT normal from A's centre: with a huge floor slab that seed lands nowhere
        // near a small box and picks the wrong feature.
        glm::vec3 candidateWorld;
        {
            glm::vec3 localBCenterInA = glm::transpose(obbA.orientation) * (centerB - centerA);
            glm::vec3 seedLocal;
            classifyLocalPoint(localBCenterInA, obbA.halfExtents, seedLocal);
            candidateWorld = centerA + obbA.orientation * seedLocal;
        }

        Feature featureA{}, featureB{};
        glm::vec3 worldClosestA(0.0f), worldClosestB(0.0f);

        constexpr int kMaxIterations = 12;
        for (int iter = 0; iter < kMaxIterations; iter++)
        {
            glm::vec3 closestLocalA, closestLocalB;

            glm::vec3 localA = glm::transpose(obbA.orientation) * (candidateWorld - centerA);
            featureA = classifyLocalPoint(localA, obbA.halfExtents, closestLocalA);
            worldClosestA = centerA + obbA.orientation * closestLocalA;

            glm::vec3 localB = glm::transpose(obbB.orientation) * (candidateWorld - centerB);
            featureB = classifyLocalPoint(localB, obbB.halfExtents, closestLocalB);
            worldClosestB = centerB + obbB.orientation * closestLocalB;

            glm::vec3 nextCandidate = 0.5f * (worldClosestA + worldClosestB);
            glm::vec3 delta = nextCandidate - candidateWorld;
            candidateWorld = nextCandidate;
            if (glm::dot(delta, delta) < 1e-8f) break;
        }

        manifold.contactPoints.clear();

        const bool aIsFace = (featureA.type == FeatureType::Face);
        const bool bIsFace = (featureB.type == FeatureType::Face);

        if (aIsFace && bIsFace)
        {
            // Genuine face-face rest: full Sutherland-Hodgman clip, exactly
            // as before - reference face A, incident face B.
            int refAxis = faceAxisOf(featureA.index);
            float refSign = faceSignOf(featureA.index);
            int u = (refAxis + 1) % 3, v = (refAxis + 2) % 3;
            glm::vec3 uAxisWorld = obbA.orientation[u];
            glm::vec3 vAxisWorld = obbA.orientation[v];

            glm::vec3 refFaceLocal(0.0f);
            refFaceLocal[refAxis] = refSign * obbA.halfExtents[refAxis];
            glm::vec3 refFaceCenterWorld = centerA + obbA.orientation * refFaceLocal;

            std::vector<glm::vec3> poly = boxFaceCorners(centerB, obbB.halfExtents, obbB.orientation,
                faceAxisOf(featureB.index), faceSignOf(featureB.index));

            poly = clipPolygonAgainstPlane(poly, refFaceCenterWorld + uAxisWorld * obbA.halfExtents[u], uAxisWorld);
            poly = clipPolygonAgainstPlane(poly, refFaceCenterWorld - uAxisWorld * obbA.halfExtents[u], -uAxisWorld);
            poly = clipPolygonAgainstPlane(poly, refFaceCenterWorld + vAxisWorld * obbA.halfExtents[v], vAxisWorld);
            poly = clipPolygonAgainstPlane(poly, refFaceCenterWorld - vAxisWorld * obbA.halfExtents[v], -vAxisWorld);

            glm::vec3 refNormalWorld = obbA.orientation[refAxis] * refSign;
            for (const glm::vec3& p : poly)
            {
                float depth = glm::dot(refFaceCenterWorld - p, refNormalWorld);
                if (depth >= -0.005f) manifold.contactPoints.push_back(p);
            }
        }
        else if (aIsFace)
        {
            auto pts = featureWorldPoints(featureB, centerB, obbB.orientation, obbB.halfExtents);
            auto clipped = clipPointsAgainstFace(pts, centerA, obbA.orientation, obbA.halfExtents, featureA.index);
            manifold.contactPoints = clipped;
        }
        else if (bIsFace)
        {
            auto pts = featureWorldPoints(featureA, centerA, obbA.orientation, obbA.halfExtents);
            auto clipped = clipPointsAgainstFace(pts, centerB, obbB.orientation, obbB.halfExtents, featureB.index);
            manifold.contactPoints = clipped;
        }

        if (manifold.contactPoints.empty())
        {
            manifold.contactPoints = { 0.5f * (worldClosestA + worldClosestB) };
        }
    }
}
