#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include "Entity/EC_DOD_Types.h"
#include "Engine/Subsystems/CollisionSystems/EC_CollisionShapes.h"

// Generic uniform-cell spatial index, keyed purely on entity id + world-space AABB.
// Deliberately has no knowledge of rendering, collision, or chunks - it's a reusable
// engine utility, not owned by any one system. Rebuilt from scratch each use (clear()
// + repeated insert()); no incremental per-entity maintenance.
class EC_SpatialGrid
{
public:
    explicit EC_SpatialGrid(float cellSize = 32.0f);

    void clear();
    void insert(EntityID entity, const AABB& worldBounds);

    // Cell-granularity query: returns every entity in any cell overlapping `region`.
    // Coarse and cheap - callers do any finer per-entity test themselves.
    std::vector<EntityID> queryAABB(const AABB& region) const;

private:
    struct CellCoord
    {
        int x, y, z;
        bool operator==(const CellCoord& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };
    struct CellCoordHash
    {
        size_t operator()(const CellCoord& c) const noexcept;
    };

    CellCoord cellCoordFor(const glm::vec3& worldPos) const;

    float m_CellSize;
    std::unordered_map<CellCoord, std::vector<EntityID>, CellCoordHash> m_Cells;
};
