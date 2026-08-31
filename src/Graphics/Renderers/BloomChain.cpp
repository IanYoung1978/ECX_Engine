#include "Graphics/Renderers/BloomChain.h"
#include <GL\glew.h>

namespace
{
    constexpr int kMinLevelDimension = 4;
}

bool BloomChain::init(int sourceWidth, int sourceHeight, int maxLevels)
{
    m_MaxLevels = maxLevels;
    allocate(sourceWidth, sourceHeight, maxLevels);
    return !m_Levels.empty();
}

void BloomChain::resize(int sourceWidth, int sourceHeight)
{
    allocate(sourceWidth, sourceHeight, m_MaxLevels);
}

void BloomChain::allocate(int sourceWidth, int sourceHeight, int maxLevels)
{
    m_Levels.clear();
    m_LevelSizes.clear();

    int w = sourceWidth / 2;
    int h = sourceHeight / 2;

    for (int i = 0; i < maxLevels && w >= kMinLevelDimension && h >= kMinLevelDimension; i++)
    {
        m_Levels.emplace_back();
        m_Levels.back().init(w, h);
        m_LevelSizes.push_back(glm::ivec2(w, h));
        w /= 2;
        h /= 2;
    }
}

void BloomChain::bindDownsampleTarget(int level)
{
    m_Levels[level].setForWriting();
}

void BloomChain::bindUpsampleTarget(int level)
{
    m_Levels[level].setForWriting();
}
