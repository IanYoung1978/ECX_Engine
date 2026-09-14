#pragma once
#include <string>


struct GameModeSettings
{
    std::string engine_settings;
    std::string controls;
    std::string scenes_file;
    std::string graphics_settings;
    std::string ui_file;
    std::string physics_materials_file;
    // Issue #130 - optional. Empty if the GameMode file has no <Prefabs> entry at all,
    // same graceful-absence convention as VoxelTerrain's `enabled` flag - a game with no
    // use for runtime spawning gets zero side effects, not a missing-file error.
    std::string prefabs_file;
};