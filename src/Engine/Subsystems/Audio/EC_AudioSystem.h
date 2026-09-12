#pragma once
#include "Engine/Subsystems/EC_System.h"
#include <memory>
#include <string>

// Generic audio playback (Issue #112) - load/play sound assets, both fire-and-forget one-
// shots (footsteps, impacts, UI clicks) and a persistent looping track (BGM), mixed through
// author-adjustable volume categories ("music"/"sfx"/"voice"/...). Built on miniaudio
// (single-header, public-domain) rather than middleware - see that issue's own notes for
// why. Pure playback mechanism; a Lua script decides what to play and when (matches the
// standing engine=mechanism/scripts=decide rule).
//
// Deliberately not exposed as a widely-included component/header (unlike e.g.
// EC_DOD_Components.h) - EC_AudioSystem.h itself is safe to include anywhere (miniaudio.h
// stays behind the pimpl below, entirely inside EC_AudioSystem.cpp), so nothing pulls in
// that ~40k-line single header just to reach this class's public surface.
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

    // A single persistent BGM-style slot, not a general N-track music system (issue #112's
    // first slice) - calling this again while a track is already loaded stops the previous
    // one first. Always routed through the "music" category.
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
