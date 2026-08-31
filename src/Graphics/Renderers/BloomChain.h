#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Graphics/FrameBuffers/FrameBuffer.h"

// A chain of progressively half-resolution FrameBuffers used for mip-chain bloom. Level 0 is
// half the source resolution, each subsequent level halves again. Downsampling walks the
// chain from level 0 to the last level; upsampling walks back from the last level to level 0,
// additively blending onto each level's existing content as it goes. After a full down-then-up
// pass, level 0 holds the final blurred bloom result.
//
// This exists because an object's bloom halo needs to scale with its actual screen footprint
// (not a fixed pixel radius) - a small/distant emissive object's signal only survives in the
// finest levels and picks up a small halo on upsample, while a large/close object's signal
// persists into coarser levels too and picks up a proportionally bigger one. See BloomChain.cpp
// and the paired bloomDownsample.frag/bloomUpsample.frag for the filter details.
class BloomChain
{
public:
    bool init(int sourceWidth, int sourceHeight, int maxLevels);
    void resize(int sourceWidth, int sourceHeight);

    int getLevelCount() const { return (int)m_Levels.size(); }
    unsigned int getLevelTexture(int level) { return m_Levels[level].getBufferTexture(); }
    unsigned int getLevelFBO(int level) { return m_Levels[level].getBufferHandle(); }
    glm::ivec2 getLevelSize(int level) const { return m_LevelSizes[level]; }

    // Binds level `level`'s FBO for writing and sets the viewport to its resolution. Caller
    // binds the downsample source texture (the original full-res glow texture for level 0,
    // getLevelTexture(level-1) otherwise) separately before drawing.
    void bindDownsampleTarget(int level);
    // Binds level `level`'s FBO for writing (existing content preserved - caller is expected to
    // have additive blending enabled) and sets the viewport. Caller binds
    // getLevelTexture(level+1) as the upsample source separately before drawing.
    void bindUpsampleTarget(int level);

private:
    void allocate(int sourceWidth, int sourceHeight, int maxLevels);

    std::vector<FrameBuffer> m_Levels;
    std::vector<glm::ivec2> m_LevelSizes;
    int m_MaxLevels = 0;
};
