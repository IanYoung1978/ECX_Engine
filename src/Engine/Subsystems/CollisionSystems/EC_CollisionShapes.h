#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>

struct OBB
{
	glm::vec3 center;       // Local center position
	glm::vec3 halfExtents;  // Half extents along each axis
	glm::mat3 orientation;  // Orientation matrix
};
struct Sphere
{
	glm::vec3 center;   // Local center position
	float radius;       // Sphere radius
};
struct Capsule
{
	glm::vec3 pointA;  // One end point of the capsule
	glm::vec3 pointB;  // Other end point of the capsule
	float radius;      // Capsule radius
};
struct AABB
{
	glm::vec3 min; // Minimum corner point
	glm::vec3 max; // Maximum corner point
};
struct Cylinder
{
	glm::vec3 center;   // Local center position
	float radius;       // Cylinder radius
	float height;       // Cylinder height
};

struct Plane
{
	glm::vec3 normal; // Plane normal vector
	float d;          // Distance from origin
};

struct Frustum
{
	glm::vec3 position;    // Center position
	glm::vec3 direction;   // Forward direction
	glm::vec3 up;          // Up direction
	glm::vec3 right;       // Right direction
	float nearPlane;      // Near plane distance
	float farPlane;       // Far plane distance
	float fov;            // Field of view in degrees
	float aspectRatio;    // Aspect ratio (width / height)
};

struct CollisionManifold
{
	std::vector<glm::vec3> contactPoints;
	glm::vec3 contactNormal;
	float penetrationDepth;
};