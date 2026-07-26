#include "CubemapShadowPool.h"
#include "Logging/ECX_Logging.h"

bool CubemapShadowPool::init(int poolSize, int faceSize)
{
    m_FaceSize = faceSize;
    m_Buffers.resize(poolSize < 0 ? 0 : poolSize);
    m_SlotUsed.assign(m_Buffers.size(), false);

    bool ok = true;
    for (auto& buffer : m_Buffers)
        ok = buffer.init(faceSize, faceSize) && ok;
    return ok;
}

bool CubemapShadowPool::acquireSlot(EntityID lightEntity)
{
    if (m_LightToSlot.find(lightEntity) != m_LightToSlot.end())
        return true;

    for (size_t i = 0; i < m_SlotUsed.size(); i++)
    {
        if (!m_SlotUsed[i])
        {
            m_SlotUsed[i] = true;
            m_LightToSlot[lightEntity] = (int)i;
            m_WarnedFull.erase(lightEntity);
            return true;
        }
    }

    if (m_WarnedFull.insert(lightEntity).second)
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "CubemapShadowPool full (" + std::to_string(m_SlotUsed.size()) +
            " slots) - point light entity " + std::to_string(lightEntity) + " will render unshadowed",
            LOGGING::LogLevel::WARNING);
    }
    return false;
}

void CubemapShadowPool::releaseSlot(EntityID lightEntity)
{
    auto it = m_LightToSlot.find(lightEntity);
    if (it == m_LightToSlot.end()) return;
    m_SlotUsed[it->second] = false;
    m_LightToSlot.erase(it);
    m_WarnedFull.erase(lightEntity);
}

bool CubemapShadowPool::hasSlot(EntityID lightEntity) const
{
    return m_LightToSlot.find(lightEntity) != m_LightToSlot.end();
}

std::vector<EntityID> CubemapShadowPool::reconcile(const std::vector<EntityID>& activeLights)
{
    std::unordered_set<EntityID> active(activeLights.begin(), activeLights.end());
    std::vector<EntityID> evicted;

    for (auto it = m_LightToSlot.begin(); it != m_LightToSlot.end(); )
    {
        if (active.find(it->first) == active.end())
        {
            m_SlotUsed[it->second] = false;
            m_WarnedFull.erase(it->first);
            evicted.push_back(it->first);
            it = m_LightToSlot.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return evicted;
}

CubemapBuffer& CubemapShadowPool::getBuffer(EntityID lightEntity)
{
    return m_Buffers[m_LightToSlot.at(lightEntity)];
}

unsigned int CubemapShadowPool::getTexture(EntityID lightEntity) const
{
    return m_Buffers[m_LightToSlot.at(lightEntity)].getTexture();
}
