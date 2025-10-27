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