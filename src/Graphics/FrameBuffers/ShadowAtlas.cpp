#include "Graphics/FrameBuffers/ShadowAtlas.h"
#include <GL\glew.h>
#include "Logging/ECX_Logging.h"

ShadowAtlas::ShadowAtlas()
{
}

ShadowAtlas::~ShadowAtlas()
{
    if (m_DepthTexture)
        glDeleteTextures(1, &m_DepthTexture);
    if (m_FBOName)
        glDeleteFramebuffers(1, &m_FBOName);
}

bool ShadowAtlas::init(int atlasSize, int tileSize)
{
    m_AtlasSize = atlasSize;
    m_TileSize = tileSize;
    m_TilesPerRow = tileSize > 0 ? (atlasSize / tileSize) : 0;
    m_TileUsed.assign((size_t)m_TilesPerRow * m_TilesPerRow, false);

    glGenFramebuffers(1, &m_FBOName);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);

    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, atlasSize, atlasSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("ShadowAtlas framebuffer incomplete", LOGGING::LogLevel::CRITICAL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool ShadowAtlas::acquireTile(EntityID lightEntity)
{
    if (m_LightToTile.find(lightEntity) != m_LightToTile.end())
        return true;

    for (size_t i = 0; i < m_TileUsed.size(); i++)
    {
        if (!m_TileUsed[i])
        {
            m_TileUsed[i] = true;
            m_LightToTile[lightEntity] = (int)i;
            m_WarnedFull.erase(lightEntity);
            return true;
        }
    }

    if (m_WarnedFull.insert(lightEntity).second)
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "ShadowAtlas full (" + std::to_string(m_TileUsed.size()) +
            " tiles) - light entity " + std::to_string(lightEntity) + " will render unshadowed",
            LOGGING::LogLevel::WARNING);
    }
    return false;
}

void ShadowAtlas::releaseTile(EntityID lightEntity)
{
    auto it = m_LightToTile.find(lightEntity);
    if (it == m_LightToTile.end()) return;
    m_TileUsed[it->second] = false;
    m_LightToTile.erase(it);
    m_WarnedFull.erase(lightEntity);
}

bool ShadowAtlas::hasTile(EntityID lightEntity) const
{
    return m_LightToTile.find(lightEntity) != m_LightToTile.end();
}

std::vector<EntityID> ShadowAtlas::reconcile(const std::vector<EntityID>& activeLights)
{
    std::unordered_set<EntityID> active(activeLights.begin(), activeLights.end());
    std::vector<EntityID> evicted;

    for (auto it = m_LightToTile.begin(); it != m_LightToTile.end(); )
    {
        if (active.find(it->first) == active.end())
        {
            m_TileUsed[it->second] = false;
            m_WarnedFull.erase(it->first);
            evicted.push_back(it->first);
            it = m_LightToTile.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return evicted;
}

void ShadowAtlas::bindTileForWriting(EntityID lightEntity)
{
    auto it = m_LightToTile.find(lightEntity);
    if (it == m_LightToTile.end()) return;

    int tx = it->second % m_TilesPerRow;
    int ty = it->second / m_TilesPerRow;
    int x = tx * m_TileSize;
    int y = ty * m_TileSize;

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);
    glViewport(x, y, m_TileSize, m_TileSize);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, m_TileSize, m_TileSize);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowAtlas::unbindTileForWriting()
{
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 ShadowAtlas::getTileBiasMatrix(EntityID lightEntity) const
{
    auto it = m_LightToTile.find(lightEntity);
    if (it == m_LightToTile.end())
    {
        return glm::mat4(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f);
    }

    int tx = it->second % m_TilesPerRow;
    int ty = it->second / m_TilesPerRow;
    float ts = (float)m_TileSize / (float)m_AtlasSize;
    float u0 = tx * ts;
    float v0 = ty * ts;

    return glm::mat4(
        0.5f * ts, 0.0f,      0.0f, 0.0f,
        0.0f,      0.5f * ts, 0.0f, 0.0f,
        0.0f,      0.0f,      0.5f, 0.0f,
        u0 + 0.5f * ts, v0 + 0.5f * ts, 0.5f, 1.0f);
}
