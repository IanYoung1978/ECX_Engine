#include "Engine/Subsystems/Audio/EC_AudioSystem.h"
#include "Logging/ECX_Logging.h"

#define MA_IMPLEMENTATION
#include "miniaudio.h"

#include <unordered_map>
#include <vector>

// Only this .cpp defines MA_IMPLEMENTATION (single-header convention, same as stb) - every
// other file that needs EC_AudioSystem includes only EC_AudioSystem.h, which never sees
// miniaudio.h at all (kept out via this pimpl struct).
struct EC_AudioSystem::Impl {
    ma_engine engine{};
    bool engineInitialised = false;

    // Heap-allocated and initialised in place (never moved/copied after
    // ma_sound_group_init): miniaudio's sound/group/node types register themselves into the
    // engine's node graph by their own address, so relocating an already-initialised one
    // (e.g. via an unordered_map rehash, or emplace copying a stack temporary in) corrupts
    // those internal pointers - this is exactly what a prior version of this code did by
    // storing ma_sound_group by value in the map, and it segfaulted immediately on the
    // first sound group created.
    std::unordered_map<std::string, std::unique_ptr<ma_sound_group>> categories;

    // Tracked purely so update() can free them once finished (ma_sound_uninit does not
    // happen automatically) - callers never see or hold these, matching the fire-and-forget
    // contract in the header.
    std::vector<std::unique_ptr<ma_sound>> oneShots;

    std::unique_ptr<ma_sound> musicSound;

    ma_sound_group* getCategory(const std::string& name) {
        auto it = categories.find(name);
        if (it != categories.end()) return it->second.get();

        auto group = std::make_unique<ma_sound_group>();
        if (ma_sound_group_init(&engine, 0, nullptr, group.get()) != MA_SUCCESS) {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "EC_AudioSystem: failed to create volume category '" + name + "'",
                LOGGING::LogLevel::SEVERE);
            return nullptr;
        }
        ma_sound_group* raw = group.get();
        categories.emplace(name, std::move(group));
        return raw;
    }
};

EC_AudioSystem::EC_AudioSystem() : m_Impl(std::make_unique<Impl>()) {
}

EC_AudioSystem::~EC_AudioSystem() {
    if (!m_Impl->engineInitialised) return;

    if (m_Impl->musicSound) ma_sound_uninit(m_Impl->musicSound.get());
    for (auto& sound : m_Impl->oneShots) ma_sound_uninit(sound.get());
    for (auto& [name, group] : m_Impl->categories) ma_sound_group_uninit(group.get());

    ma_engine_uninit(&m_Impl->engine);
}

void EC_AudioSystem::init(ECXMessenger& messenger, EC_Game& game) {
    if (ma_engine_init(nullptr, &m_Impl->engine) != MA_SUCCESS) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_AudioSystem: ma_engine_init failed - audio will be unavailable",
            LOGGING::LogLevel::CRITICAL);
        return;
    }
    m_Impl->engineInitialised = true;
}

void EC_AudioSystem::update(const float& deltaTimeS, EC_Game& game) {
    if (!m_Impl->engineInitialised) return;

    // Reap finished one-shots so the list doesn't grow unbounded - miniaudio keeps mixing
    // whatever is still playing regardless of when we get around to freeing finished ones.
    auto& oneShots = m_Impl->oneShots;
    for (size_t i = 0; i < oneShots.size();) {
        if (ma_sound_at_end(oneShots[i].get())) {
            ma_sound_uninit(oneShots[i].get());
            oneShots[i] = std::move(oneShots.back());
            oneShots.pop_back();
        } else {
            i++;
        }
    }
}

void EC_AudioSystem::playSound(const std::string& path, float volume, const std::string& category) {
    if (!m_Impl->engineInitialised) return;

    ma_sound_group* group = m_Impl->getCategory(category);
    if (!group) return;

    auto sound = std::make_unique<ma_sound>();
    ma_uint32 flags = MA_SOUND_FLAG_DECODE;
    if (ma_sound_init_from_file(&m_Impl->engine, path.c_str(), flags, group, nullptr, sound.get()) != MA_SUCCESS) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_AudioSystem: failed to load sound '" + path + "'", LOGGING::LogLevel::SEVERE);
        return;
    }

    ma_sound_set_volume(sound.get(), volume);
    ma_sound_start(sound.get());
    m_Impl->oneShots.push_back(std::move(sound));
}

void EC_AudioSystem::playMusic(const std::string& path, float volume, bool loop) {
    if (!m_Impl->engineInitialised) return;

    stopMusic();

    ma_sound_group* group = m_Impl->getCategory("music");
    if (!group) return;

    auto sound = std::make_unique<ma_sound>();
    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM;
    if (ma_sound_init_from_file(&m_Impl->engine, path.c_str(), flags, group, nullptr, sound.get()) != MA_SUCCESS) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_AudioSystem: failed to load music '" + path + "'", LOGGING::LogLevel::SEVERE);
        return;
    }

    ma_sound_set_looping(sound.get(), loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), volume);
    ma_sound_start(sound.get());
    m_Impl->musicSound = std::move(sound);
}

void EC_AudioSystem::stopMusic() {
    if (!m_Impl->musicSound) return;
    ma_sound_uninit(m_Impl->musicSound.get());
    m_Impl->musicSound.reset();
}

void EC_AudioSystem::setCategoryVolume(const std::string& category, float volume) {
    if (!m_Impl->engineInitialised) return;

    if (category == "master") {
        ma_engine_set_volume(&m_Impl->engine, volume);
        return;
    }

    ma_sound_group* group = m_Impl->getCategory(category);
    if (!group) return;
    ma_sound_group_set_volume(group, volume);
}
