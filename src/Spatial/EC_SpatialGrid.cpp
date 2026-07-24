#include "EC_SpatialGrid.h"
#include <unordered_set>
#include <cmath>

EC_SpatialGrid::EC_SpatialGrid(float cellSize)
    : m_CellSize(cellSize)
{
}

void EC_SpatialGrid::clear()
{
    m_Cells.clear();
}

EC_SpatialGrid::CellCoord EC_SpatialGrid::cellCoordFor(const glm::vec3& worldPos) const
{
    return CellCoord{
        static_cast<int>(std::floor(worldPos.x / m_CellSize)),
        static_cast<int>(std::floor(worldPos.y / m_CellSize)),
        static_cast<int>(std::floor(worldPos.z / m_CellSize))
    };
}

size_t EC_SpatialGrid::CellCoordHash::operator()(const CellCoord& c) const noexcept
{
    size_t h1 = std::hash<int>()(c.x);
    size_t h2 = std::hash<int>()(c.y);
    size_t h3 = std::hash<int>()(c.z);
    return h1 ^ (h2 * 0x9E3779B97F4A7C15ULL) ^ (h3 * 0xC2B2AE3D27D4EB4FULL);
}

void EC_SpatialGrid::insert(EntityID entity, const AABB& worldBounds)
{
    CellCoord minCell = cellCoordFor(worldBounds.min);
    CellCoord maxCell = cellCoordFor(worldBounds.max);

    for (int x = minCell.x; x <= maxCell.x; x++)
        for (int y = minCell.y; y <= maxCell.y; y++)
            for (int z = minCell.z; z <= maxCell.z; z++)
                m_Cells[CellCoord{ x, y, z }].push_back(entity);
}

std::vector<EntityID> EC_SpatialGrid::queryAABB(const AABB& region) const
{
    CellCoord minCell = cellCoordFor(region.min);
    CellCoord maxCell = cellCoordFor(region.max);

    std::unordered_set<EntityID> found;
    for (int x = minCell.x; x <= maxCell.x; x++)
    {
        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int z = minCell.z; z <= maxCell.z; z++)
            {
                auto it = m_Cells.find(CellCoord{ x, y, z });
                if (it != m_Cells.end())
                    found.insert(it->second.begin(), it->second.end());
            }
        }
    }
    return std::vector<EntityID>(found.begin(), found.end());
}
