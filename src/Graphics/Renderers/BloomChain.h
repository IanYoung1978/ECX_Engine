#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Graphics/FrameBuffers/FrameBuffer.h"

// Chain of progressively half-resolution FrameBuffers for mip-chain bloom. Level 0 is half the
// source; downsampling walks 0..last, upsampling walks back additively, leaving the result in
// level 0. The halo scales with an object's screen footprint.
class BloomChain
{
public:
    bool init(int sourceWidth, int sourceHeight, int maxLevels);
    void resize(int sourceWidth, int sourceHeight);

    int getLevelCount() const { return (int)m_Levels.size(); }
    unsigned int getLevelTexture(int level) { return m_Levels[level].getBufferTexture(); }
    unsigned int getLevelFBO(int level) { return m_Levels[level].getBufferHandle(); }
    glm::ivec2 getLevelSize(int level) const { return m_LevelSizes[level]; }

    // Binds level's FBO for writing and sets the viewport. The caller binds the downsample source (the
    // full-res glow texture for level 0, getLevelTexture(level-1) otherwise).
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
