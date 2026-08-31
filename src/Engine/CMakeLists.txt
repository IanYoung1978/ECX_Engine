target_sources(ECX PRIVATE
    Config.h
    EC_Engine.cpp
    EC_Engine.h
    GameModeSettings.h
    Timer.cpp
    Timer.h
)

add_subdirectory(Controllers)
add_subdirectory(Subsystems)
