#pragma once
#include <glm/glm.hpp>

// Params for visualizing the last ray/cone query via GL_DebugRenderer, shared by EC_GameAPI.h and
// the renderers so one struct can travel in an ECXCommand's std::any.
struct DebugRayVisualization
{
    glm::vec3 origin{ 0.0f };
    glm::vec3 direction{ 0.0f };
    float maxDistance = 0.0f;
};

struct DebugConeVisualization
{
    glm::vec3 apex{ 0.0f };
    glm::vec3 direction{ 0.0f };
    float halfAngleRadians = 0.0f;
    float maxDistance = 0.0f;
};
