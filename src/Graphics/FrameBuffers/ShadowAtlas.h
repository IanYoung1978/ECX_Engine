#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <glm/glm.hpp>
#include "Entity/EC_DOD_Types.h"

// One depth texture divided into a fixed grid of equal tiles. Each light keeps a stable tile until
// it stops being an active caster, so all active lights' shadows exist for the whole frame.
class ShadowAtlas
{
public:
    ShadowAtlas();
    ~ShadowAtlas();

    bool init(int atlasSize, int tileSize);

    // Assigns a stable tile to lightEntity if it doesn't already have one. Returns false
    // (hard reject, no eviction) if the atlas is full - caller should skip the depth
    // render for this light this frame and treat it as unshadowed.
    bool acquireTile(EntityID lightEntity);
    void releaseTile(EntityID lightEntity);
    bool hasTile(EntityID lightEntity) const;

    // Frees the tile of any light not present in activeLights. Returns the EntityIDs that
    // were actually freed this call, so callers can invalidate any per-light caches (e.g.
    // static-light shadow baking) keyed on tile ownership.
    std::vector<EntityID> reconcile(const std::vector<EntityID>& activeLights);

    // Binds the atlas FBO, restricts viewport+scissor to lightEntity's tile, and clears
    // just that tile's depth. Must have an acquired tile already (see acquireTile).
    void bindTileForWriting(EntityID lightEntity);
    // Disables the scissor test and unbinds the FBO. Must be called after every
    // bindTileForWriting - a leaked scissor rect silently clips all later rendering to
    // the last tile's rect.
    void unbindTileForWriting();

    // Remaps the [0,1] bias matrix into lightEntity's tile of the atlas, so the sampler2Shadow shaders
    // still sample [0,1] unmodified.
    glm::mat4 getTileBiasMatrix(EntityID lightEntity) const;

    unsigned int getDepthTexture() const { return m_DepthTexture; }
    int getTileSize() const { return m_TileSize; }
    int getTileCount() const { return m_TilesPerRow * m_TilesPerRow; }

private:
    unsigned int m_FBOName = 0;
    unsigned int m_DepthTexture = 0;
    int m_AtlasSize = 0;
    int m_TileSize = 0;
    int m_TilesPerRow = 0;
    std::vector<bool> m_TileUsed;
    std::unordered_map<EntityID, int> m_LightToTile;
    std::unordered_set<EntityID> m_WarnedFull;
};
