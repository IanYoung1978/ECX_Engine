#pragma once
#include "Engine/Subsystems/EC_System.h"
#include <memory>
#include <string>

// Sound playback: one-shots and a looping music track, mixed through volume categories.
// Mechanism only; scripts decide what to play. miniaudio stays behind the pimpl in the .cpp so
// this header is cheap to include.
class EC_AudioSystem : public EC_System {
public:
    EC_AudioSystem();
    virtual ~EC_AudioSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;

    // Fire-and-forget: starts playing immediately, mixed independently of any other sound
    // (including other calls to this same function) - safe to call rapidly/repeatedly, e.g.
    // once per footstep, without needing to track or stop anything. category is created on
    // first use if it doesn't exist yet (see setCategoryVolume) and defaults to full volume.
    void playSound(const std::string& path, float volume, const std::string& category);

    // A single persistent music slot: calling this again stops the previous track. Always routed
    // through the "music" category.
    void playMusic(const std::string& path, float volume, bool loop);
    void stopMusic();

    // "master" is the engine-wide output volume (affects every category); any other name
    // is a category created on first use by this call or by playSound/playMusic, whichever
    // happens first - either way starting at full volume until adjusted.
    void setCategoryVolume(const std::string& category, float volume);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};
