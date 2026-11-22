# ECX_Engine

ECX_Engine is a high-performance game engine designed for creating immersive and interactive experiences.

## Features
- Advanced rendering capabilities
- Cross-platform support
- Modular entity component system
- Optimized performance
- Scalable design
- Customizable components
- Multithreading support


## prerequisites
- C++17 compatible compiler
- CMake 3.10 or higher
- OpenGL 4.2 or higher
- Git


### Windows specific
- vcpkg package manager

### Linux specific
- apt-get or yum package manager

### macos specific
- Homebrew package manager
- chocolatey package manager


## Installation

### Windows
To install dependencies, follow these steps:

- Install dependencies using vcpkg:
 ```
   vcpkg install glm sdl2 assimp Lua Lubraidge3 glew sdl-image
  ```

### Linux
To install dependencies, use the following commands based on your package manager:
- For apt-get:
```
 sudo apt-get install libglm-dev libsdl2-dev libassimp-dev liblua5.3-dev libglew-dev libsdl2-image-dev
 ```
 - For yum:
 ```
   sudo yum install glm-devel SDL2-devel assimp-devel lua-devel glew-devel SDL2_image-devel
 ```


### MacOS
To install dependencies, use the following commands based on your package manager:
- For Homebrew:
 ```
   brew install glm sdl2 assimp lua glew sdl2_image
 ```

## Building the Engine

1. Clone the repository:
   ```
   git clone https://github.com/IanYoung1978/ECX_Engine.git
2. Create a build directory and navigate into it:
   ```
   mkdir build && cd build
   ```
3. Configure the project using CMake:
   ```
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
   ```
4. Build the project:
   ```
   cmake --build .
   ```
5. Run the engine:
   ```
   ./ECX_Engine
   ```

## How the engine runs

The engine steps three fixed 1/60 s loops on their own threads, plus the main thread:

- **Physics thread:** each tick runs Spatial, Transform and Camera once, then Collision and Physics `Substeps` times.
- **Scripting thread:** all Lua handlers run here.
- **Audio thread:** sound bookkeeping only; mixing runs on miniaudio's own thread.
- **Main thread:** input, message dispatch, scene loading, terrain uploads and rendering.

Events, commands and requests are queued and delivered on the next main-thread frame, so a handler that triggers another event does not recurse.

## Engine configuration

`data/scripts/XML/EngineConfig.xml`:

| Element | Effect |
|---|---|
| `<Startup pauseOnStart>` | Pause after the first scene loads (default `true`). |
| `<Physics><Substeps>` | Collision and Physics passes per tick. Higher is more stable stacking; 1 disables substepping. |
| `<Physics><Debug>` | `LogEnergy`, `LogVelocity`, `LogAngularVelocity`, `LogFriction` write per-body debug lines to the log. |
| `<EntityLifecycle quarantineFrames>` | Frames a deactivated scene's entities are kept before being destroyed (default 30). |
| `<ComponentStorage initialCapacity growthThreshold>` | Per-component-type capacity (default 512) and how full it gets before growing. A "reached capacity" log warning means raise it. |
| `<VoxelTerrain enabled script>` | Voxel terrain is off unless enabled; `script` generates it. |
| `<DebugHTTP enabled port>` | Local debug server, off by default (port 8089). |

`<Physics><TimeStep>` and the `<Collision>` algorithm elements are currently not read; the tick rate is fixed at 60 Hz.

`data/scripts/XML/GraphicsSetting.xml`:

| Element | Effect |
|---|---|
| `<DirShadow distance>` | How far the directional-shadow frustum reaches along the camera's view. |
| `<DirShadow biasScale>` | Scales the shadow bias that prevents self-shadowing. The bias needed grows with shadow-map texel size (roughly shadow box width / atlas tile size); the defaults suit `distance="50"` with a 1024 tile, so scale up for larger scenes or smaller tiles. |
| `<AmbientScale>` | Strength of the flat ambient fill, which is derived each frame from the active lights' average colour. |

## Rigid bodies and collision events

`<RigidBody><BodyType>` is one of:

- `Static`: never moves.
- `Kinematic`: moved by scripts. The impulse solver never touches it, but it still gets collision events and gravity.
- `Dynamic`: fully simulated (default).

`<IgnoreGravity>` is independent of the body type. Scripts receive `onCollisionBegin` and `onCollisionEnd`. While a Kinematic body moves and stays in contact, `onCollisionBegin` fires again each tick with a fresh contact normal and penetration depth (`event:getCollisionNormal()`, `event:getCollisionPenetrationDepth()`).

## Scenes

An entity is `Active`, `Inactive` or `MarkedForDeletion`. Only active entities take part in updates, collision and rendering. Switching scenes deactivates the old scene's entities; they are destroyed after `quarantineFrames`.

## Debugging

- `log.html` in the working directory is written on shutdown. In Debug builds a failed assertion or unhandled exception also writes its stack trace there before the usual dialog.
- With `<DebugHTTP enabled="true">`, `GET /docs` lists the routes: `/log`, `/components`, `/screenshot`, `/resize`, `/rayQuery`, `/coneQuery`, `/capsuleQuery`, `/regenerateTerrain` and the window toggles.
- `EventHandlerReference.md` documents the Lua API.
