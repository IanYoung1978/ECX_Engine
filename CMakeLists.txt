cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
# attach to vcpkg's cmake configuration for project dependencies
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message("Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
    set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
endif()

project(ECX LANGUAGES CXX)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

# Off in CI: lets ECX_UnitTests configure with only glm + Catch2.
option(ECX_BUILD_ENGINE "Build the main ECX game target (requires SDL2/GLEW/assimp/Lua/OpenGL/Stb)" ON)

set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_BUILD_TYPE Debug)

find_package(GLM CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

if(ECX_BUILD_ENGINE)

# Explicit sources, not a glob - each src/ subdirectory owns a CMakeLists.txt.
add_executable(ECX
    src/Eclipsis.cpp
    src/Game.cpp
    src/Game.h
    src/resource.h
)

add_subdirectory(src/Components)
add_subdirectory(src/Engine)
add_subdirectory(src/Entity)
add_subdirectory(src/Graphics)
add_subdirectory(src/Logging)
add_subdirectory(src/Messaging)
add_subdirectory(src/SceneManager)
add_subdirectory(src/Spatial)
add_subdirectory(src/TaskManager)
add_subdirectory(src/UI)
add_subdirectory(src/vendor)
add_subdirectory(src/Window)
add_subdirectory(src/xml)

set_property(TARGET ECX PROPERTY CXX_STANDARD 17)

set(INCLUDE_DIRS "")
set(LIBS "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLOG_ENABLED")

#set(CMAKE_FIND_DEBUG_MODE TRUE)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(OpenGL REQUIRED)

#set(CMAKE_FIND_DEBUG_MODE FALSE)


if (SDL2_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${SDL2_LIBRARIES})
    message("SDL found at: ${SDL2_INCLUDE_DIRS}")
    message("SDL libs: ${SDL2_LIBRARIES}")
else()
    message("SDL2 not found: sudo apt install libsdl2-dev" FATAL_ERROR)
	message("SDL2 not found: vcpkg install sdl2" FATAL_ERROR)
endif()

if (SDL2_image_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} SDL2_image::SDL2_image)
    set(LIBS ${LIBS} SDL2_image::SDL2_image)
    message("SDL2_image found")
    message("SDL2_image libs")
else()
    message("SDL2 not found: sudo apt install libsdl2-dev" FATAL_ERROR)
	message("SDL2 not found: vcpkg install sdl2" FATAL_ERROR)
endif()

if (GLEW_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${GLEW_LIBRARIES})
    message("GLEW found at: ${GLEW_INCLUDE_DIRS}")
    message("GLEW libs: ${GLEW_LIBRARIES}")
else()
    message("GLEW not found: sudo apt install libglew-dev" FATAL_ERROR)
    message("GLEW not found: vcpkg install glew" FATAL_ERROR)
endif()

if (OPENGL_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${OPENGL_LIBRARIES})
    message("OPENGL found at: ${OPENGL_INCLUDE_DIRS}")
    message("OPENGL libs: ${OPENGL_LIBRARIES}")
else()
    message("OPENGL not found: sudo apt install opengl" FATAL_ERROR)
    message("OPENGL not found: vcpkg install opengl" FATAL_ERROR)
endif()

if (GLM_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} glm::glm)
    set(LIBS ${LIBS} glm::glm)
else()
    message("GLM not found: sudo apt install libglm-dev " FATAL_ERROR)
	message("GLM not found: vcpkg install glm" FATAL_ERROR)
endif()

if (assimp_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} assimp::assimp)
    set(LIBS ${LIBS} assimp::assimp)
else()
    message("Assimp not found: sudo apt install libassimp-dev" FATAL_ERROR)
    message("Assimp not found: vcpkginstall assimp" FATAL_ERROR)
endif()

if (LUA_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
    set(LIBS ${LIBS} ${LUA_LIBRARIES})
    message("Lua found at: ${LUA_INCLUDE_DIR}")
    message("Lua libs: ${LUA_LIBRARIES}")
else()
    message("Lua not found: sudo apt install Lua" FATAL_ERROR)
    message("Lua not found: vcpkg install Lua" FATAL_ERROR)
endif()

find_package(Stb REQUIRED)
if (Stb_FOUND)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${Stb_INCLUDE_DIR})
    message("Stb found at: ${Stb_INCLUDE_DIR}")
else()
    message("Stb not found: vcpkg install stb" FATAL_ERROR)
endif()

find_path(LUABRIDGE3_INCLUDE_DIRS "luabridge3/LuaBridge/Array.h")
message(${LUABRIDGE3_INCLUDE_DIRS})
set(INCLUDE_DIRS ${INCLUDE_DIRS} ${LUABRIDGE3_INCLUDE_DIRS})

target_include_directories(ECX PRIVATE ${INCLUDE_DIRS} ${SOURCE_DIR})
target_link_libraries(ECX ${LIBS})

add_custom_command(
        TARGET ECX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data
                ${CMAKE_CURRENT_BINARY_DIR}/data)
add_custom_command(
        TARGET ECX PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/data
                ${CMAKE_CURRENT_BINARY_DIR}/data
        COMMENT "Copying data files...")

endif() # ECX_BUILD_ENGINE

# Pure-logic collision/spatial unit tests - only depends on glm + Catch2, not the engine.
add_executable(ECX_UnitTests
    tests/EC_GJK_Tests.cpp
    tests/EC_CollisionChecks_Tests.cpp
    tests/EC_VClip_Tests.cpp
    tests/EC_PairManager_Tests.cpp
    tests/EC_SpatialGrid_Tests.cpp
    tests/EC_Frustum_Tests.cpp
    src/Engine/Subsystems/CollisionSystems/EC_GJK.cpp
    src/Engine/Subsystems/CollisionSystems/EC_ConvexSupport.cpp
    src/Engine/Subsystems/CollisionSystems/EC_CollisionChecks.cpp
    src/Engine/Subsystems/CollisionSystems/EC_VClip.cpp
    src/Engine/Subsystems/CollisionSystems/EC_PairManager.cpp
    src/Spatial/EC_SpatialGrid.cpp
    src/Spatial/EC_Frustum.cpp
)
set_property(TARGET ECX_UnitTests PROPERTY CXX_STANDARD 17)
target_include_directories(ECX_UnitTests PRIVATE ${SOURCE_DIR})
target_link_libraries(ECX_UnitTests PRIVATE glm::glm Catch2::Catch2WithMain)