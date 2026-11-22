#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Entity/EC_DOD_Types.h"
#include "CubemapBuffer.h"

// Persistent pool of point-light shadow cubemaps. Each light keeps a stable slot until it stops
// being an active caster, so up to poolSize lights have shadows at once. Slots are independent
// CubemapBuffers sampled by direction, so no UV remapping is needed (unlike ShadowAtlas).
class CubemapShadowPool
{
public:
    bool init(int poolSize, int faceSize);

    // Assigns a stable slot to lightEntity if it doesn't already have one. Returns false
    // (hard reject, no eviction) if the pool is full - caller should skip the depth
    // render for this light this frame and treat it as unshadowed.
    bool acquireSlot(EntityID lightEntity);
    void releaseSlot(EntityID lightEntity);
    bool hasSlot(EntityID lightEntity) const;

    // Frees the slot of any light not present in activeLights. Returns the EntityIDs that
    // were actually freed this call.
    std::vector<EntityID> reconcile(const std::vector<EntityID>& activeLights);

    CubemapBuffer& getBuffer(EntityID lightEntity);
    unsigned int getTexture(EntityID lightEntity) const;

private:
    std::vector<CubemapBuffer> m_Buffers;
    std::vector<bool> m_SlotUsed;
    std::unordered_map<EntityID, int> m_LightToSlot;
    std::unordered_set<EntityID> m_WarnedFull;
    int m_FaceSize = 0;
};
