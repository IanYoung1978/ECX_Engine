#include "EC_GJK.h"
#include <array>
#include <vector>

namespace
{
    // Conservative advancement (raycast) converges linearly, not quadratically, for
    // near-tangential/grazing hits - a ray just clipping a sphere's edge can need well
    // over 32 steps to reach a tight epsilon. Boolean intersects() converges much
    // faster (it only needs to prove/disprove overlap, not pin down an exact point) but
    // shares the same cap for simplicity.
    constexpr int kMaxIterations = 64;
}

namespace
{
    glm::vec3 minkowskiSupport(const EC_GJK::SupportFn& a, const EC_GJK::SupportFn& b, const glm::vec3& dir)
    {
        return a(dir) - b(-dir);
    }

    bool sameDirection(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::dot(a, b) > 0.0f;
    }

    // --- Boolean GJK (Minkowski-difference simplex evolution) ---------------------

    struct Simplex
    {
        std::array<glm::vec3, 4> points{};
        int count = 0;
    };

    bool lineCase(Simplex& s, glm::vec3& dir)
    {
        const glm::vec3& a = s.points[0];
        const glm::vec3& b = s.points[1];
        glm::vec3 ab = b - a;
        glm::vec3 ao = -a;

        if (sameDirection(ab, ao)) {
            dir = glm::cross(glm::cross(ab, ao), ab);
        } else {
            s = Simplex{ { a }, 1 };
            dir = ao;
        }
        return false;
    }

    bool triangleCase(Simplex& s, glm::vec3& dir)
    {
        glm::vec3 a = s.points[0], b = s.points[1], c = s.points[2];
        glm::vec3 ab = b - a, ac = c - a, ao = -a;
        glm::vec3 abc = glm::cross(ab, ac);

        if (sameDirection(glm::cross(abc, ac), ao)) {
            if (sameDirection(ac, ao)) {
                s = Simplex{ { a, c }, 2 };
                dir = glm::cross(glm::cross(ac, ao), ac);
            } else {
                s = Simplex{ { a, b }, 2 };
                return lineCase(s, dir);
            }
            return false;
        }
        if (sameDirection(glm::cross(ab, abc), ao)) {
            s = Simplex{ { a, b }, 2 };
            return lineCase(s, dir);
        }
        if (sameDirection(abc, ao)) {
            dir = abc;
        } else {
            s = Simplex{ { a, c, b }, 3 };
            dir = -abc;
        }
        return false;
    }

    bool tetrahedronCase(Simplex& s, glm::vec3& dir)
    {
        glm::vec3 a = s.points[0], b = s.points[1], c = s.points[2], d = s.points[3];
        glm::vec3 ab = b - a, ac = c - a, ad = d - a, ao = -a;

        glm::vec3 abc = glm::cross(ab, ac);
        glm::vec3 acd = glm::cross(ac, ad);
        glm::vec3 adb = glm::cross(ad, ab);

        if (sameDirection(abc, ao)) { s = Simplex{ { a, b, c }, 3 }; return triangleCase(s, dir); }
        if (sameDirection(acd, ao)) { s = Simplex{ { a, c, d }, 3 }; return triangleCase(s, dir); }
        if (sameDirection(adb, ao)) { s = Simplex{ { a, d, b }, 3 }; return triangleCase(s, dir); }
        return true; // origin enclosed
    }

    bool nextSimplex(Simplex& s, glm::vec3& dir)
    {
        switch (s.count) {
        case 2: return lineCase(s, dir);
        case 3: return triangleCase(s, dir);
        case 4: return tetrahedronCase(s, dir);
        default: return false;
        }
    }

    // --- Closest point on triangle to the origin (Ericson, RTCD 5.1.5), specialized
    // for p = (0,0,0). Returns the closest point and which of the triangle's original
    // vertices (by index 0/1/2) remain part of the closest feature. --------------------

    struct TriangleClosest
    {
        glm::vec3 point;
        std::vector<int> retained;
    };

    TriangleClosest closestOnTriangleToOrigin(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        glm::vec3 ab = b - a, ac = c - a, ap = -a;
        float d1 = glm::dot(ab, ap), d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f) return { a, { 0 } };

        glm::vec3 bp = -b;
        float d3 = glm::dot(ab, bp), d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) return { b, { 1 } };

        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return { a + v * ab, { 0, 1 } };
        }

        glm::vec3 cp = -c;
        float d5 = glm::dot(ab, cp), d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return { c, { 2 } };

        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return { a + w * ac, { 0, 2 } };
        }

        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return { b + w * (c - b), { 1, 2 } };
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom, w = vc * denom;
        return { a + ab * v + ac * w, { 0, 1, 2 } };
    }
}

namespace EC_GJK
{
    bool intersects(const SupportFn& supportA, const SupportFn& supportB)
    {
        glm::vec3 dir(1.0f, 0.0f, 0.0f);
        glm::vec3 p0 = minkowskiSupport(supportA, supportB, dir);

        Simplex simplex{ { p0 }, 1 };
        dir = -p0;

        for (int iter = 0; iter < kMaxIterations; iter++)
        {
            if (glm::dot(dir, dir) < 1e-12f)
                return true; // origin coincides with the simplex already

            glm::vec3 p = minkowskiSupport(supportA, supportB, dir);
            if (glm::dot(p, dir) < 0.0f)
                return false; // support point didn't pass the origin - shapes are separated

            // push_front
            for (int i = std::min(simplex.count, 3); i > 0; i--)
                simplex.points[i] = simplex.points[i - 1];
            simplex.points[0] = p;
            simplex.count = std::min(simplex.count + 1, 4);

            if (nextSimplex(simplex, dir))
                return true;
        }
        return false; // did not converge within the iteration budget - treat as separated
    }

    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float tmax,
        const SupportFn& support, float& outT, glm::vec3& outNormal)
    {
        const float EPS = 1e-5f;
        float lambda = 0.0f;
        glm::vec3 x = origin;
        glm::vec3 n(0.0f);

        std::vector<glm::vec3> pPoints; // support points on the shape (w = x - p, recomputed as x moves)

        glm::vec3 v = x - support(glm::vec3(1.0f, 0.0f, 0.0f));
        if (glm::dot(v, v) < EPS * EPS)
            v = glm::vec3(1.0f, 0.0f, 0.0f);

        for (int iter = 0; iter < kMaxIterations; iter++)
        {
            glm::vec3 p = support(v);
            glm::vec3 w = x - p;

            if (glm::dot(v, w) > 0.0f)
            {
                if (glm::dot(v, dir) >= 0.0f)
                    return false; // ray can never close the gap in this direction
                lambda -= glm::dot(v, w) / glm::dot(v, dir);
                if (lambda > tmax)
                    return false;
                x = origin + dir * lambda;
                n = v;
                // The simplex was built from support points extreme relative to the OLD
                // x - after jumping x to a new position along the ray, those points no
                // longer describe anything meaningful about x's relationship to the
                // shape (recomputing w = x - p keeps them numerically valid but
                // geometrically stale), so the point-to-shape distance search must
                // restart fresh from here rather than accumulate them into a 3-point
                // simplex alongside the new point below.
                pPoints.clear();
            }

            pPoints.push_back(p);
            if (pPoints.size() > 3)
                pPoints.erase(pPoints.begin()); // keep the 3 most recent (defensive cap)

            std::vector<glm::vec3> wPoints;
            wPoints.reserve(pPoints.size());
            for (const glm::vec3& pt : pPoints)
                wPoints.push_back(x - pt);

            if (wPoints.size() == 1)
            {
                v = wPoints[0];
            }
            else if (wPoints.size() == 2)
            {
                glm::vec3 a = wPoints[0], b = wPoints[1];
                glm::vec3 ab = b - a;
                float denom = glm::dot(ab, ab);
                float t = (denom > EPS) ? glm::clamp(glm::dot(-a, ab) / denom, 0.0f, 1.0f) : 0.0f;
                v = a + t * ab;
                if (t <= 0.0f) pPoints = { pPoints[0] };
                else if (t >= 1.0f) pPoints = { pPoints[1] };
            }
            else // 3 points
            {
                TriangleClosest result = closestOnTriangleToOrigin(wPoints[0], wPoints[1], wPoints[2]);
                v = result.point;
                std::vector<glm::vec3> reduced;
                reduced.reserve(result.retained.size());
                for (int idx : result.retained)
                    reduced.push_back(pPoints[idx]);
                pPoints = reduced;
            }

            if (glm::dot(v, v) < EPS * EPS)
            {
                outT = lambda;
                outNormal = (glm::dot(n, n) > EPS) ? glm::normalize(n) : -dir;
                return true;
            }
        }
        return false; // did not converge within the iteration budget
    }
}
