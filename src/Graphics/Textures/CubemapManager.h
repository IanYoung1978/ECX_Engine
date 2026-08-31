#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <GL/glew.h>

class CubemapManager
{
public:
    CubemapManager() = default;
    ~CubemapManager();

    // Phase 1: load raw pixel data on loading thread (no GL)
    void loadHDR(const std::string& filename);

    // Phase 2: upload to GL and convert to cubemap on main thread
    void finalizeAll();

    // Get cubemap handle by filename
    unsigned int getCubemap(const std::string& filename);

private:
    struct PendingHDR {
        std::string filename;
        float* pixels = nullptr;
        int width = 0;
        int height = 0;
    };

    unsigned int convertEquirectToCubemap(float* pixels, int width, int height);
    void buildCubeVAO();

    std::vector<PendingHDR> m_Pending;
    std::unordered_map<std::string, unsigned int> m_Cubemaps;

    unsigned int m_CubeVAO = 0;
    unsigned int m_CubeVBO = 0;
};