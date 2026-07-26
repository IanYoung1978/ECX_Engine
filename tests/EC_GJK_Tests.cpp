// Standalone unit tests for EC_GJK / EC_ConvexSupport (Issues #30/#29), isolated from
// the engine (no SDL/OpenGL/ECS/Lua dependency - just glm). Build/run via the
// ECX_GJK_Tests CMake target. No test framework dependency - plain CHECK() macro
// tracking pass/fail counts, non-zero exit code on any failure.
#include "Engine/Subsystems/CollisionSystems/EC_GJK.h"
#include "Engine/Subsystems/CollisionSystems/EC_ConvexSupport.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cmath>

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void check(bool condition, const char* description)
    {
        if (condition) {
            g_passed++;
        } else {
            g_failed++;
            std::printf("FAIL: %s\n", description);
        }
    }

    void checkNear(float actual, float expected, float tolerance, const char* description)
    {
        check(std::abs(actual - expected) <= tolerance, description);
    }

    void checkNear(const glm::vec3& actual, const glm::vec3& expected, float tolerance, const char* description)
    {
        check(glm::length(actual - expected) <= tolerance, description);
    }

    using namespace EC_ConvexSupport;

    EC_GJK::SupportFn sphereFn(const glm::vec3& c, float r)
    {
        return [c, r](const glm::vec3& d) { return sphereSupport(c, r, d); };
    }

    EC_GJK::SupportFn boxFn(const glm::vec3& c, const glm::vec3& he, const glm::mat3& rot)
    {
        return [c, he, rot](const glm::vec3& d) { return boxSupport(c, he, rot, d); };
    }

    EC_GJK::SupportFn capsuleFn(const glm::vec3& a, const glm::vec3& b, float r)
    {
        return [a, b, r](const glm::vec3& d) { return capsuleSupport(a, b, r, d); };
    }

    EC_GJK::SupportFn cylinderFn(const glm::vec3& a, const glm::vec3& b, float r)
    {
        return [a, b, r](const glm::vec3& d) { return cylinderSupport(a, b, r, d); };
    }

    EC_GJK::SupportFn coneFn(const glm::vec3& apex, const glm::vec3& axis, float halfAngleRad, float height)
    {
        return [apex, axis, halfAngleRad, height](const glm::vec3& d) {
            return coneSupport(apex, axis, halfAngleRad, height, d);
        };
    }

    void test_intersects_sphere_sphere()
    {
        check(EC_GJK::intersects(sphereFn({ 0, 0, 0 }, 1.0f), sphereFn({ 1.5f, 0, 0 }, 1.0f)),
            "sphere-sphere: overlapping (dist 1.5, radii 1+1) should intersect");
        check(!EC_GJK::intersects(sphereFn({ 0, 0, 0 }, 1.0f), sphereFn({ 3.0f, 0, 0 }, 1.0f)),
            "sphere-sphere: separated (dist 3, radii 1+1) should not intersect");
    }

    void test_intersects_box_box()
    {
        glm::mat3 identity(1.0f);
        check(EC_GJK::intersects(
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
            boxFn({ 1.5f, 0, 0 }, { 1, 1, 1 }, identity)),
            "box-box: overlapping (dist 1.5, half-extents 1+1) should intersect");
        check(!EC_GJK::intersects(
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
            boxFn({ 3.0f, 0, 0 }, { 1, 1, 1 }, identity)),
            "box-box: separated (dist 3, half-extents 1+1) should not intersect");
    }

    void test_intersects_obb_aabb()
    {
        glm::mat3 identity(1.0f);
        glm::mat3 rot45 = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0)));
        // Box rotated 45deg around Y with half-extents (1,1,1) reaches ~1.41421 along
        // world X (cos45 + sin45). Combined with the AABB's own 1.0 half-extent, the
        // overlap threshold along X is ~2.41421.
        check(EC_GJK::intersects(
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
            boxFn({ 1.8f, 0, 0 }, { 1, 1, 1 }, rot45)),
            "obb-aabb: overlapping (dist 1.8 < ~2.414 reach) should intersect");
        check(!EC_GJK::intersects(
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
            boxFn({ 3.5f, 0, 0 }, { 1, 1, 1 }, rot45)),
            "obb-aabb: separated (dist 3.5 > ~2.414 reach) should not intersect");
    }

    void test_intersects_cone_sphere()
    {
        glm::vec3 apex(0, 0, 0), axis(0, 0, 1);
        float halfAngle = glm::radians(20.0f);
        float height = 50.0f;

        check(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn({ 0, 0, 10 }, 1.0f)),
            "cone-sphere: dead ahead, unobstructed should intersect");

        // 45deg off-axis, small radius - well outside a 20deg cone.
        glm::vec3 offAxisPos = glm::vec3(std::sin(glm::radians(45.0f)), 0, std::cos(glm::radians(45.0f))) * 10.0f;
        check(!EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn(offAxisPos, 1.0f)),
            "cone-sphere: 45deg off-axis, outside 20deg cone should not intersect");

        // Center at ~24.2deg (outside a 20deg cone) but radius 2 large enough that its
        // near edge dips inside the cone's ~3.64-radius cross-section at that depth -
        // this is the "partially hit should be collected" case.
        glm::vec3 partialPos(4.5f, 0, 10.0f);
        check(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn(partialPos, 2.0f)),
            "cone-sphere: center outside angle but radius overlaps cone edge should intersect");
    }

    void test_intersects_cone_box()
    {
        // Reproduces the reported bug: a box whose center is outside the cone's angle
        // but whose near corner pokes inside it.
        glm::vec3 apex(0, 0, 0), axis(0, 0, 1);
        float halfAngle = glm::radians(20.0f);
        float height = 50.0f;
        glm::mat3 identity(1.0f);

        // Box center (5,0,10) is at atan(5/10)=26.57deg (outside 20deg), but its near
        // corner at x=5-2=3 is at atan(3/10)=16.7deg (inside 20deg).
        check(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height),
            boxFn({ 5, 0, 10 }, { 2, 2, 2 }, identity)),
            "cone-box: center outside angle, near corner inside cone should intersect");

        // Box entirely outside even at its nearest corner (x=9, atan(9/10)=41.9deg).
        check(!EC_GJK::intersects(coneFn(apex, axis, halfAngle, height),
            boxFn({ 10, 0, 10 }, { 1, 1, 1 }, identity)),
            "cone-box: entirely outside cone angle (including near corner) should not intersect");
    }

    void test_raycast_sphere()
    {
        float t; glm::vec3 n;
        bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, sphereFn({ 0, 0, 0 }, 2.0f), t, n);
        check(hit, "raycast-sphere: should hit");
        checkNear(t, 8.0f, 0.01f, "raycast-sphere: distance should be 8 (origin -10, surface at -2)");
        checkNear(n, { -1, 0, 0 }, 0.01f, "raycast-sphere: normal should point back toward origin");

        bool miss = EC_GJK::raycast({ -10, 5, 0 }, { 1, 0, 0 }, 100.0f, sphereFn({ 0, 0, 0 }, 2.0f), t, n);
        check(!miss, "raycast-sphere: ray offset beyond radius should miss");
    }

    void test_raycast_aabb()
    {
        float t; glm::vec3 n;
        glm::mat3 identity(1.0f);
        bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f,
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity), t, n);
        check(hit, "raycast-aabb: should hit");
        checkNear(t, 9.0f, 0.01f, "raycast-aabb: distance should be 9 (origin -10, face at -1)");

        bool miss = EC_GJK::raycast({ -10, 5, 0 }, { 1, 0, 0 }, 100.0f,
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity), t, n);
        check(!miss, "raycast-aabb: ray offset beyond extents should miss");
    }

    void test_raycast_obb()
    {
        float t; glm::vec3 n;
        glm::mat3 rot45 = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0)));
        bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f,
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, rot45), t, n);
        check(hit, "raycast-obb: should hit");
        // Reach along X for a 45deg-rotated unit-half-extent box is cos45+sin45 ~= 1.41421.
        checkNear(t, 10.0f - 1.41421f, 0.02f, "raycast-obb: distance should account for rotated silhouette");
    }

    void test_raycast_capsule()
    {
        float t; glm::vec3 n;
        glm::vec3 a(0, -2, 0), b(0, 2, 0);

        // Through the cylindrical body.
        bool hitBody = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, capsuleFn(a, b, 1.0f), t, n);
        check(hitBody, "raycast-capsule: body hit should succeed");
        checkNear(t, 9.0f, 0.01f, "raycast-capsule: body hit distance should be 9");

        // Through the rounded top cap only - capsule axis runs along Y, so "above the
        // straight segment" means offsetting Y past b.y=2, not Z.
        bool hitCap = EC_GJK::raycast({ -10, 2.5f, 0 }, { 1, 0, 0 }, 100.0f, capsuleFn(a, b, 1.0f), t, n);
        check(hitCap, "raycast-capsule: cap hit should succeed");
        checkNear(t, 9.134f, 0.05f, "raycast-capsule: cap hit distance should be ~9.134");
    }

    void test_raycast_cylinder()
    {
        float t; glm::vec3 n;
        glm::vec3 a(0, -2, 0), b(0, 2, 0);

        // Straight up through the flat bottom cap.
        bool hitCap = EC_GJK::raycast({ 0, -10, 0 }, { 0, 1, 0 }, 100.0f, cylinderFn(a, b, 1.0f), t, n);
        check(hitCap, "raycast-cylinder: flat cap hit should succeed");
        checkNear(t, 8.0f, 0.01f, "raycast-cylinder: flat cap hit distance should be 8");
        checkNear(n, { 0, -1, 0 }, 0.01f, "raycast-cylinder: flat cap normal should face down");

        // Through the round side.
        bool hitBody = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, cylinderFn(a, b, 1.0f), t, n);
        check(hitBody, "raycast-cylinder: body hit should succeed");
        checkNear(t, 9.0f, 0.01f, "raycast-cylinder: body hit distance should be 9");
    }
}

int main()
{
    test_intersects_sphere_sphere();
    test_intersects_box_box();
    test_intersects_obb_aabb();
    test_intersects_cone_sphere();
    test_intersects_cone_box();
    test_raycast_sphere();
    test_raycast_aabb();
    test_raycast_obb();
    test_raycast_capsule();
    test_raycast_cylinder();

    std::printf("\n%d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
