#include "EC_CollisionChecks.h"
#include "EC_VClip.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace {
    constexpr float kEpsilon = 1e-6f;

    glm::vec3 safeNormalize(const glm::vec3& v, const glm::vec3& fallback = glm::vec3(0.0f, 1.0f, 0.0f)) {
        const float len2 = glm::dot(v, v);
        if (len2 <= kEpsilon) {
            return fallback;
        }
        return v / std::sqrt(len2);
    }

    glm::vec3 clampVec3(const glm::vec3& v, const glm::vec3& minV, const glm::vec3& maxV) {
        return glm::vec3(
            std::clamp(v.x, minV.x, maxV.x),
            std::clamp(v.y, minV.y, maxV.y),
            std::clamp(v.z, minV.z, maxV.z));
    }

    AABB toWorldAABB(const AABB& aabb, const glm::vec3& pos) {
        return { aabb.min + pos, aabb.max + pos };
    }

    // Closest point on segment [a,b] to point p - standard projection-and-clamp.
    glm::vec3 closestPointOnSegment(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
        const glm::vec3 ab = b - a;
        const float abLen2 = glm::dot(ab, ab);
        if (abLen2 <= kEpsilon) return a; // degenerate (zero-length) segment
        float t = glm::dot(p - a, ab) / abLen2;
        t = std::clamp(t, 0.0f, 1.0f);
        return a + t * ab;
    }

    // Closest point between a segment [segA,segB] and an axis-aligned box: alternately
    // clamp a point on the segment into the box, then clamp that result back onto the
    // segment. Both shapes are convex, so this converges in a handful of iterations -
    // there's no simple closed form for segment-vs-box the way there is for point-vs-box
    // or segment-vs-segment, and real-time narrow-phase use doesn't need an exact one.
    // Returns the closest point ON THE SEGMENT via the return value, and the closest
    // point IN THE BOX via outPointInBox.
    glm::vec3 closestPointSegmentAABB(const glm::vec3& segA, const glm::vec3& segB,
        const glm::vec3& boxMin, const glm::vec3& boxMax, glm::vec3& outPointInBox) {
        glm::vec3 pointOnSegment = (segA + segB) * 0.5f;
        glm::vec3 pointInBox = clampVec3(pointOnSegment, boxMin, boxMax);
        for (int i = 0; i < 4; i++) {
            pointOnSegment = closestPointOnSegment(pointInBox, segA, segB);
            glm::vec3 nextPointInBox = clampVec3(pointOnSegment, boxMin, boxMax);
            if (glm::dot(nextPointInBox - pointInBox, nextPointInBox - pointInBox) < 1e-10f) {
                pointInBox = nextPointInBox;
                break;
            }
            pointInBox = nextPointInBox;
        }
        outPointInBox = pointInBox;
        return pointOnSegment;
    }

    // Closest points between two segments (p1,q1) and (p2,q2) - standard algorithm
    // (Ericson, "Real-Time Collision Detection" 5.1.9), including its degenerate handling
    // for zero-length segments and near-parallel lines.
    void closestPointsSegmentSegment(const glm::vec3& p1, const glm::vec3& q1,
        const glm::vec3& p2, const glm::vec3& q2, glm::vec3& c1, glm::vec3& c2) {
        const glm::vec3 d1 = q1 - p1;
        const glm::vec3 d2 = q2 - p2;
        const glm::vec3 r = p1 - p2;
        const float a = glm::dot(d1, d1);
        const float e = glm::dot(d2, d2);
        const float f = glm::dot(d2, r);

        float s, t;
        if (a <= kEpsilon && e <= kEpsilon) {
            c1 = p1; c2 = p2; return;
        }
        if (a <= kEpsilon) {
            s = 0.0f;
            t = std::clamp(f / e, 0.0f, 1.0f);
        } else {
            const float c = glm::dot(d1, r);
            if (e <= kEpsilon) {
                t = 0.0f;
                s = std::clamp(-c / a, 0.0f, 1.0f);
            } else {
                const float b = glm::dot(d1, d2);
                const float denom = a * e - b * b;
                s = (denom > kEpsilon) ? std::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
                t = (b * s + f) / e;
                if (t < 0.0f) {
                    t = 0.0f;
                    s = std::clamp(-c / a, 0.0f, 1.0f);
                } else if (t > 1.0f) {
                    t = 1.0f;
                    s = std::clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }
        c1 = p1 + d1 * s;
        c2 = p2 + d2 * t;
    }

    // Closest point on triangle (a,b,c) to point p - standard algorithm (Ericson,
    // "Real-Time Collision Detection" 5.1.5), classifying p into one of the triangle's 7
    // Voronoi regions (3 vertices, 3 edges, the face interior).
    glm::vec3 closestPointOnTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        const glm::vec3 ab = b - a;
        const glm::vec3 ac = c - a;
        const glm::vec3 ap = p - a;
        const float d1 = glm::dot(ab, ap);
        const float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f) return a;

        const glm::vec3 bp = p - b;
        const float d3 = glm::dot(ab, bp);
        const float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) return b;

        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            const float v = d1 / (d1 - d3);
            return a + v * ab;
        }

        const glm::vec3 cp = p - c;
        const float d5 = glm::dot(ab, cp);
        const float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return c;

        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            const float w = d2 / (d2 - d6);
            return a + w * ac;
        }

        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + w * (c - b);
        }

        const float denom = 1.0f / (va + vb + vc);
        const float v = vb * denom;
        const float w = vc * denom;
        return a + ab * v + ac * w;
    }

    // Closest points between segment [s0,s1] and triangle (a,b,c). When the segment does
    // NOT cross the triangle's plane within the triangle's interior, the minimum distance
    // pair is always either a segment-endpoint-vs-triangle-interior pair or a
    // segment-vs-triangle-edge pair - covered by the endpoint/edge checks below. But when
    // the segment actually PENETRATES the triangle (its interior crosses the plane at a
    // point inside the triangle - exactly the "capsule embedded in/resting on a flat
    // surface" case), the true zero-distance closest pair is that interior crossing point,
    // which lies strictly BETWEEN the two endpoints - neither endpoint/edge check can ever
    // find it. Missing this case was the actual root cause of the capsule "bounce" bug: for
    // a flat quad split into two triangles, whichever triangle's endpoint/edge-only
    // candidate happened to come out marginally closer would win the global-minimum pick,
    // even though its winning point wasn't the true closest (perpendicular) pair - producing
    // a bogus non-vertical "contact normal" that flip-flopped between triangles frame to
    // frame as the capsule's Y position drifted by fractions of a unit.
    void closestPointsSegmentTriangle(const glm::vec3& s0, const glm::vec3& s1,
        const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
        glm::vec3& outSegPoint, glm::vec3& outTriPoint) {
        float bestDist2 = std::numeric_limits<float>::max();

        auto consider = [&](const glm::vec3& segPt, const glm::vec3& triPt) {
            const glm::vec3 d = triPt - segPt;
            const float dist2 = glm::dot(d, d);
            if (dist2 < bestDist2) {
                bestDist2 = dist2;
                outSegPoint = segPt;
                outTriPoint = triPt;
            }
        };

        consider(s0, closestPointOnTriangle(s0, a, b, c));
        consider(s1, closestPointOnTriangle(s1, a, b, c));

        const glm::vec3 edgeStarts[3] = { a, b, c };
        const glm::vec3 edgeEnds[3] = { b, c, a };
        for (int i = 0; i < 3; i++) {
            glm::vec3 segPt, edgePt;
            closestPointsSegmentSegment(s0, s1, edgeStarts[i], edgeEnds[i], segPt, edgePt);
            consider(segPt, edgePt);
        }

        // Segment-vs-triangle-plane penetration check (see comment above).
        const glm::vec3 ab = b - a;
        const glm::vec3 ac = c - a;
        const glm::vec3 normal = glm::cross(ab, ac);
        const float normalLen2 = glm::dot(normal, normal);
        if (normalLen2 > kEpsilon) {
            const glm::vec3 seg = s1 - s0;
            const float denom = glm::dot(normal, seg);
            if (std::abs(denom) > kEpsilon) {
                const float t = glm::dot(normal, a - s0) / denom;
                if (t >= 0.0f && t <= 1.0f) {
                    const glm::vec3 p = s0 + seg * t;
                    // Barycentric inside-triangle test (Ericson 3.4).
                    const glm::vec3 ap = p - a;
                    const float d00 = glm::dot(ab, ab);
                    const float d01 = glm::dot(ab, ac);
                    const float d11 = glm::dot(ac, ac);
                    const float d20 = glm::dot(ap, ab);
                    const float d21 = glm::dot(ap, ac);
                    const float baryDenom = d00 * d11 - d01 * d01;
                    if (std::abs(baryDenom) > kEpsilon) {
                        const float v = (d11 * d20 - d01 * d21) / baryDenom;
                        const float w = (d00 * d21 - d01 * d20) / baryDenom;
                        const float u = 1.0f - v - w;
                        if (u >= 0.0f && v >= 0.0f && w >= 0.0f) {
                            consider(p, p);
                        }
                    }
                }
            }
        }
    }
}


bool EC_CollisionChecks::SphereVsSphere(const Sphere& sphereA, const glm::vec3& posA, const Sphere& sphereB, const glm::vec3& posB, CollisionManifold& manifold)
{
    const glm::vec3 centerA = posA + sphereA.center;
    const glm::vec3 centerB = posB + sphereB.center;
    const glm::vec3 delta = centerB - centerA;
    const float dist2 = glm::dot(delta, delta);
    const float radiusSum = sphereA.radius + sphereB.radius;

    if (dist2 > radiusSum * radiusSum) {
        return false;
    }

    const float distance = std::sqrt(std::max(dist2, 0.0f));
    const glm::vec3 normal = (distance > kEpsilon) ? (delta / distance) : glm::vec3(1.0f, 0.0f, 0.0f);
    const float penetration = radiusSum - distance;
    const glm::vec3 contact = centerA + normal * (sphereA.radius - penetration * 0.5f);

    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints = { contact };
    return true;

}

bool EC_CollisionChecks::AABBVsAABB(const AABB& aabbA, const glm::vec3& posA, const AABB& aabbB, const glm::vec3& posB, CollisionManifold& manifold)
{
    const AABB wa = toWorldAABB(aabbA, posA);
    const AABB wb = toWorldAABB(aabbB, posB);

    if (wa.max.x < wb.min.x || wa.min.x > wb.max.x ||
        wa.max.y < wb.min.y || wa.min.y > wb.max.y ||
        wa.max.z < wb.min.z || wa.min.z > wb.max.z) {
        return false;
    }

    const float overlapX = std::min(wa.max.x, wb.max.x) - std::max(wa.min.x, wb.min.x);
    const float overlapY = std::min(wa.max.y, wb.max.y) - std::max(wa.min.y, wb.min.y);
    const float overlapZ = std::min(wa.max.z, wb.max.z) - std::max(wa.min.z, wb.min.z);

    glm::vec3 normal(1.0f, 0.0f, 0.0f);
    float penetration = overlapX;

    const glm::vec3 centerA = (wa.min + wa.max) * 0.5f;
    const glm::vec3 centerB = (wb.min + wb.max) * 0.5f;

    if (overlapY < penetration) {
        penetration = overlapY;
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (overlapZ < penetration) {
        penetration = overlapZ;
        normal = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    const glm::vec3 delta = centerB - centerA;
    if (glm::dot(delta, normal) < 0.0f) {
        normal = -normal;
    }

    const glm::vec3 overlapMin = glm::max(wa.min, wb.min);
    const glm::vec3 overlapMax = glm::min(wa.max, wb.max);

    // Contact face: the 4 corners of the overlap region in the plane
    // perpendicular to the collision normal (both boxes are axis-aligned, so
    // this is exact - no clipping needed, unlike the general OBB case).
    // Multiple points let a resting/toppled box get counter-torque from all
    // 4 corners at once, instead of pivoting/rolling around one.
    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints.clear();

    if (normal.y != 0.0f) {
        const float y = (overlapMin.y + overlapMax.y) * 0.5f;
        manifold.contactPoints = {
            { overlapMin.x, y, overlapMin.z },
            { overlapMax.x, y, overlapMin.z },
            { overlapMax.x, y, overlapMax.z },
            { overlapMin.x, y, overlapMax.z },
        };
    }
    else if (normal.x != 0.0f) {
        const float x = (overlapMin.x + overlapMax.x) * 0.5f;
        manifold.contactPoints = {
            { x, overlapMin.y, overlapMin.z },
            { x, overlapMax.y, overlapMin.z },
            { x, overlapMax.y, overlapMax.z },
            { x, overlapMin.y, overlapMax.z },
        };
    }
    else {
        const float z = (overlapMin.z + overlapMax.z) * 0.5f;
        manifold.contactPoints = {
            { overlapMin.x, overlapMin.y, z },
            { overlapMax.x, overlapMin.y, z },
            { overlapMax.x, overlapMax.y, z },
            { overlapMin.x, overlapMax.y, z },
        };
    }

    return true;

}
bool EC_CollisionChecks::OBBVsOBB(const OBB& obbA, const glm::vec3& posA,
    const OBB& obbB, const glm::vec3& posB,
    CollisionManifold& manifold)
{
    glm::vec3 centerA = posA + obbA.center;
    glm::vec3 centerB = posB + obbB.center;

    // Get the 3 axes of each OBB
    glm::vec3 axisA[3] = {
        obbA.orientation[0],
        obbA.orientation[1],
        obbA.orientation[2]
    };

    glm::vec3 axisB[3] = {
        obbB.orientation[0],
        obbB.orientation[1],
        obbB.orientation[2]
    };

    // Vector between centers
    glm::vec3 T = centerB - centerA;

    float ra, rb;
    glm::mat3 R, absR;

    // Compute rotation matrix expressing B in A's coordinate frame
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(axisA[i], axisB[j]);
        }
    }

    // Compute translation vector T in A's frame
    glm::vec3 t(glm::dot(T, axisA[0]), glm::dot(T, axisA[1]), glm::dot(T, axisA[2]));

    // Compute absolute values with epsilon for numerical stability
    const float epsilon = 0.00001f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            absR[i][j] = glm::abs(R[i][j]) + epsilon;
        }
    }

    // Track the axis with the smallest penetration across all 15 candidate
    // axes (standard SAT-derived manifold technique) - that axis becomes the
    // contact normal. Unlike the boolean-only test, cross-product (edge-edge)
    // axes must be normalized here so their overlap is a real distance,
    // comparable against the face axes.
    float bestOverlap = 1e30f;
    glm::vec3 bestAxis(0.0f);

    // Test axes L = A0, A1, A2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[i];
        rb = obbB.halfExtents[0] * absR[i][0] + obbB.halfExtents[1] * absR[i][1] + obbB.halfExtents[2] * absR[i][2];
        float overlap = ra + rb - glm::abs(t[i]);
        if (overlap < 0.0f) return false;
        if (overlap < bestOverlap) { bestOverlap = overlap; bestAxis = axisA[i]; }
    }

    // Test axes L = B0, B1, B2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[0] * absR[0][i] + obbA.halfExtents[1] * absR[1][i] + obbA.halfExtents[2] * absR[2][i];
        rb = obbB.halfExtents[i];
        float proj = t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i];
        float overlap = ra + rb - glm::abs(proj);
        if (overlap < 0.0f) return false;
        if (overlap < bestOverlap) { bestOverlap = overlap; bestAxis = axisB[i]; }
    }

    // Test the 9 edge-edge cross-product axes L = Ai x Bj. For axis-aligned
    // (or near-axis-aligned) boxes - e.g. a cube resting flat on the floor -
    // several of these 9 axes are mathematically identical to a face axis
    // already tested above (cross(X,Y) is the same line as the Z face axis),
    // just computed through a different formula path with different
    // floating-point rounding. Without a tie-breaking bias, that rounding
    // noise can make the "duplicate" edge-edge overlap come out marginally
    // smaller than the true face overlap and win the naive minimum
    // comparison - which permanently misclassifies a flat face-face rest as
    // a single-point edge-edge contact, and a box resting on one point
    // instead of a stable 4-point face can only pivot/rotate freely about
    // its own centre rather than being held flat. kEdgeEdgeBias requires an
    // edge-edge axis to beat the best face overlap by a real margin, not
    // just numerically, before it's allowed to override a face axis -
    // standard SAT practice for exactly this degeneracy. Kept deliberately
    // small: the true floating-point noise between the duplicate axis-
    // aligned computations is only ~1e-5 to 1e-4 at this engine's unit
    // scale, so this only needs to be comfortably above that - not so
    // large that it starts overriding a GENUINE edge/vertex contact for a
    // meaningfully tilted box (e.g. a corner striking the floor mid-topple)
    // in favour of an incorrect face axis, which would understate that
    // corner's true penetration depth and let it sink before the impulse
    // solver reacts properly.
    constexpr float kEdgeEdgeBias = 0.001f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 axis = glm::cross(axisA[i], axisB[j]);
            float len2 = glm::dot(axis, axis);
            if (len2 < 1e-8f) continue; // near-parallel edges - degenerate, skip

            const float invLen = 1.0f / std::sqrt(len2);
            axis *= invLen;

            float raGen = 0.0f, rbGen = 0.0f;
            for (int k = 0; k < 3; k++) {
                raGen += obbA.halfExtents[k] * glm::abs(glm::dot(axisA[k], axis));
                rbGen += obbB.halfExtents[k] * glm::abs(glm::dot(axisB[k], axis));
            }
            float overlap = raGen + rbGen - glm::abs(glm::dot(T, axis));
            if (overlap < 0.0f) return false;
            if (overlap + kEdgeEdgeBias < bestOverlap) {
                bestOverlap = overlap; bestAxis = axis;
            }
        }
    }

    // No separating axis found - OBBs are colliding. bestAxis/bestOverlap now
    // hold the minimum-penetration axis and depth.
    glm::vec3 normal = bestAxis;
    if (glm::dot(T, normal) < 0.0f) normal = -normal; // orient A -> B

    manifold.contactNormal = normal;
    manifold.penetrationDepth = bestOverlap;

    // SAT above is only used for the boolean test and penetration depth -
    // both robust regardless of body size. Which FEATURE pair (face/edge/
    // vertex on each box) is actually touching is picked independently here,
    // by walking to the true closest features via local Voronoi-region
    // classification rather than SAT's raw numeric overlap comparison, which
    // misclassifies "edge-edge" whenever body sizes are grossly mismatched
    // (e.g. a cube vs a room-sized floor slab) - see EC_VClip.h for why.
    EC_VClip::generateContactPoints(obbA, posA, obbB, posB, normal, bestOverlap, manifold);

    return true;
}

bool EC_CollisionChecks::SphereVsAABB(const Sphere& sphere, const glm::vec3& spherePos, const AABB& aabb, const glm::vec3& aabbPos, CollisionManifold& manifold)
{
    const glm::vec3 center = spherePos + sphere.center;
    const AABB waabb = toWorldAABB(aabb, aabbPos);

    const glm::vec3 closest = clampVec3(center, waabb.min, waabb.max);
    const glm::vec3 delta = center - closest;
    const float dist2 = glm::dot(delta, delta);

    if (dist2 > sphere.radius * sphere.radius) {
        return false;
    }

    const float dist = std::sqrt(std::max(dist2, 0.0f));
    const glm::vec3 normal = (dist > kEpsilon) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);

    manifold.contactNormal = normal;
    manifold.penetrationDepth = sphere.radius - dist;
    manifold.contactPoints = { closest };
    return true;

}

bool EC_CollisionChecks::SphereVsOBB(const Sphere& sphere, const glm::vec3& spherePos, const OBB& obb, const glm::vec3& obbPos, CollisionManifold& manifold)
{
    const glm::vec3 worldCenter = spherePos + sphere.center;
    const glm::vec3 obbCenter = obbPos + obb.center;

    const glm::mat3 orientation = obb.orientation;
    const glm::mat3 invOrientation = glm::transpose(orientation);

    const glm::vec3 localSphere = invOrientation * (worldCenter - obbCenter);
    const glm::vec3 localClosest = clampVec3(localSphere, -obb.halfExtents, obb.halfExtents);
    const glm::vec3 localDelta = localSphere - localClosest;
    const float dist2 = glm::dot(localDelta, localDelta);

    if (dist2 > sphere.radius * sphere.radius) {
        return false;
    }

    const glm::vec3 worldClosest = obbCenter + orientation * localClosest;
    const glm::vec3 worldDelta = worldCenter - worldClosest;
    const float dist = std::sqrt(std::max(glm::dot(worldDelta, worldDelta), 0.0f));

    manifold.contactNormal = safeNormalize(worldDelta);
    manifold.penetrationDepth = sphere.radius - dist;
    manifold.contactPoints = { worldClosest };
    return true;

}

bool EC_CollisionChecks::FrustumVsAABB(const Frustum& frustum, const glm::vec3& frustumPos, const AABB& aabb, const glm::vec3& aabbPos)
{
    // Conservative approximation for now: frustum bound as sphere.
    const float halfFovRad = glm::radians(frustum.fov * 0.5f);
    const float frustumRadius = std::max(frustum.farPlane * std::tan(halfFovRad), frustum.farPlane);

    const glm::vec3 frustumCenter = frustumPos + frustum.direction * (frustum.farPlane * 0.5f);
    const glm::vec3 boxCenter = ((aabb.min + aabb.max) * 0.5f) + aabbPos;
    const glm::vec3 boxExtents = (aabb.max - aabb.min) * 0.5f;

    const glm::vec3 delta = frustumCenter - boxCenter;
    const glm::vec3 closest = clampVec3(delta, -boxExtents, boxExtents);
    const glm::vec3 diff = delta - closest;
    return glm::dot(diff, diff) <= frustumRadius * frustumRadius;

}

bool EC_CollisionChecks::CapsuleVsSphere(const Capsule& capsule, const glm::vec3& capsulePos,
    const Sphere& sphere, const glm::vec3& spherePos, CollisionManifold& manifold)
{
    const glm::vec3 a = capsulePos + capsule.pointA;
    const glm::vec3 b = capsulePos + capsule.pointB;
    const glm::vec3 sphereCenter = spherePos + sphere.center;

    const glm::vec3 closest = closestPointOnSegment(sphereCenter, a, b);
    const glm::vec3 delta = sphereCenter - closest;
    const float dist2 = glm::dot(delta, delta);
    const float radiusSum = capsule.radius + sphere.radius;

    if (dist2 > radiusSum * radiusSum) {
        return false;
    }

    const float dist = std::sqrt(std::max(dist2, 0.0f));
    // Points toward the sphere (the second parameter) - EC_NarrowPhase flips this when
    // the sphere ends up as dispatch-parameter A, same as SphereVsAABB/SphereVsOBB flip
    // when the sphere is dispatch-A there.
    const glm::vec3 normal = (dist > kEpsilon) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);
    const float penetration = radiusSum - dist;

    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints = { closest + normal * (capsule.radius - penetration * 0.5f) };
    return true;
}

bool EC_CollisionChecks::CapsuleVsAABB(const Capsule& capsule, const glm::vec3& capsulePos,
    const AABB& aabb, const glm::vec3& aabbPos, CollisionManifold& manifold)
{
    const glm::vec3 a = capsulePos + capsule.pointA;
    const glm::vec3 b = capsulePos + capsule.pointB;
    const AABB box = toWorldAABB(aabb, aabbPos);

    glm::vec3 pointInBox;
    const glm::vec3 pointOnSegment = closestPointSegmentAABB(a, b, box.min, box.max, pointInBox);

    const glm::vec3 delta = pointInBox - pointOnSegment; // toward the box (second parameter)
    const float dist2 = glm::dot(delta, delta);

    if (dist2 > capsule.radius * capsule.radius) {
        return false;
    }

    const float dist = std::sqrt(std::max(dist2, 0.0f));
    manifold.contactNormal = (dist > kEpsilon) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);
    manifold.penetrationDepth = capsule.radius - dist;
    manifold.contactPoints = { pointInBox };
    return true;
}

bool EC_CollisionChecks::CapsuleVsOBB(const Capsule& capsule, const glm::vec3& capsulePos,
    const OBB& obb, const glm::vec3& obbPos, CollisionManifold& manifold)
{
    const glm::vec3 worldA = capsulePos + capsule.pointA;
    const glm::vec3 worldB = capsulePos + capsule.pointB;
    const glm::vec3 obbCenter = obbPos + obb.center;

    const glm::mat3 orientation = obb.orientation;
    const glm::mat3 invOrientation = glm::transpose(orientation);

    const glm::vec3 localA = invOrientation * (worldA - obbCenter);
    const glm::vec3 localB = invOrientation * (worldB - obbCenter);

    glm::vec3 localPointInBox;
    const glm::vec3 localPointOnSegment = closestPointSegmentAABB(
        localA, localB, -obb.halfExtents, obb.halfExtents, localPointInBox);

    const glm::vec3 localDelta = localPointOnSegment - localPointInBox;
    const float dist2 = glm::dot(localDelta, localDelta);

    if (dist2 > capsule.radius * capsule.radius) {
        return false;
    }

    const glm::vec3 worldPointInBox = obbCenter + orientation * localPointInBox;
    const glm::vec3 worldPointOnSegment = obbCenter + orientation * localPointOnSegment;
    const glm::vec3 worldDelta = worldPointInBox - worldPointOnSegment; // toward the OBB (second parameter)
    const float dist = std::sqrt(std::max(glm::dot(worldDelta, worldDelta), 0.0f));

    manifold.contactNormal = safeNormalize(worldDelta);
    manifold.penetrationDepth = capsule.radius - dist;
    manifold.contactPoints = { worldPointInBox };
    return true;
}

bool EC_CollisionChecks::CapsuleVsMesh(const Capsule& capsule, const glm::vec3& capsulePos,
    const std::vector<glm::vec3>& meshPositions, const std::vector<glm::vec3>& meshNormals,
    const std::vector<uint32_t>& meshIndices,
    const glm::vec3& meshPos, CollisionManifold& manifold)
{
    const glm::vec3 segA = capsulePos + capsule.pointA;
    const glm::vec3 segB = capsulePos + capsule.pointB;

    float bestDist2 = std::numeric_limits<float>::max();
    glm::vec3 bestSegPoint(0.0f), bestMeshPoint(0.0f), bestFaceNormal(0.0f, 1.0f, 0.0f);
    bool found = false;

    for (size_t i = 0; i + 2 < meshIndices.size(); i += 3) {
        const glm::vec3 a = meshPos + meshPositions[meshIndices[i]];
        const glm::vec3 b = meshPos + meshPositions[meshIndices[i + 1]];
        const glm::vec3 c = meshPos + meshPositions[meshIndices[i + 2]];

        glm::vec3 segPoint, triPoint;
        closestPointsSegmentTriangle(segA, segB, a, b, c, segPoint, triPoint);

        const glm::vec3 d = triPoint - segPoint;
        const float dist2 = glm::dot(d, d);
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            bestSegPoint = segPoint;
            bestMeshPoint = triPoint;
            // Average of the winning triangle's 3 vertex gradient normals - see this
            // function's header comment for why this, not a cross-product winding-based
            // normal, is what's reliable here (a marching-cubes winding parity bug can
            // flip individual triangles' winding even on a flat, continuous surface; the
            // gradient normals don't depend on winding at all).
            const glm::vec3& na = meshNormals[meshIndices[i]];
            const glm::vec3& nb = meshNormals[meshIndices[i + 1]];
            const glm::vec3& nc = meshNormals[meshIndices[i + 2]];
            bestFaceNormal = safeNormalize(na + nb + nc, bestFaceNormal);
            found = true;
        }
    }

    if (!found || bestDist2 > capsule.radius * capsule.radius) {
        return false;
    }

    const float dist = std::sqrt(std::max(bestDist2, 0.0f));

    manifold.contactNormal = bestFaceNormal;
    manifold.penetrationDepth = capsule.radius - dist;
    manifold.contactPoints = { bestMeshPoint };
    return true;
}

bool EC_CollisionChecks::CapsuleVsCapsule(const Capsule& capsuleA, const glm::vec3& posA,
    const Capsule& capsuleB, const glm::vec3& posB, CollisionManifold& manifold)
{
    const glm::vec3 a1 = posA + capsuleA.pointA;
    const glm::vec3 b1 = posA + capsuleA.pointB;
    const glm::vec3 a2 = posB + capsuleB.pointA;
    const glm::vec3 b2 = posB + capsuleB.pointB;

    glm::vec3 c1, c2;
    closestPointsSegmentSegment(a1, b1, a2, b2, c1, c2);

    const glm::vec3 delta = c2 - c1; // capsuleA -> capsuleB, self-orienting like SphereVsSphere
    const float dist2 = glm::dot(delta, delta);
    const float radiusSum = capsuleA.radius + capsuleB.radius;

    if (dist2 > radiusSum * radiusSum) {
        return false;
    }

    const float dist = std::sqrt(std::max(dist2, 0.0f));
    const glm::vec3 normal = (dist > kEpsilon) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);
    const float penetration = radiusSum - dist;

    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints = { c1 + normal * (capsuleA.radius - penetration * 0.5f) };
    return true;
}
