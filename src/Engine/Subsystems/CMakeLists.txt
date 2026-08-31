target_sources(ECX PRIVATE
    EC_System.cpp
    EC_System.h
    EC_SystemType.h
)

add_subdirectory(Camera)
add_subdirectory(CollisionSystems)
add_subdirectory(Control)
add_subdirectory(Scripting)
add_subdirectory(Spatial)
add_subdirectory(Transform)
