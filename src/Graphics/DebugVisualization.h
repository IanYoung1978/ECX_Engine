#pragma once
#include <glm/glm.hpp>

// Params for visualizing the last ray/cone query (Issues #30/#29) via GL_DebugRenderer.
// Shared between EC_GameAPI.h (script-facing setter) and GL_Deferred_Renderer/
// GL_DebugRenderer (the consumer) so a single struct can travel through an
// ECXCommand's std::any payload.
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
