# ECX_Engine

ECX_Engine is a high-performance game engine designed for creating immersive and interactive experiences.

## Features
- Advanced rendering capabilities
- Cross-platform support
- Modular architecture
- Regular updates
- Optimized performance
- Scalable design
- Customizable components
- Multithreading support
- Scripting support
- Code samples

## prerequisites
- C++17 compatible compiler
- CMake 3.10 or higher
- OpenGL 4.2 or higher
- vcpkg package manager
- Git


## Installation
To install ECX_Engine, follow these steps:
1. Clone the repository:
   ```
   git clone https://github.com/IanYoung1978/ECX_Engine.git
2. Install dependencies using vcpkg:
   ```
   vcpkg install glm sdl2 assimp Lua Lubraidge3 glew sdl-image
   ```
3. Create a build directory and navigate into it:
   ```
   mkdir build && cd build
   ```
4. Configure the project using CMake:
   ```
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
   ```
5. Build the project:
   ```
   cmake --build .
   ```
6. Run the engine:
   ```
   ./ECX_Engine
   ```