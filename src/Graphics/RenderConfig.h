#pragma once

struct RenderConfig
{
    float exposure = 0.75f;
    float emissiveIntensity = 2.0f;
    int bloomMipLevels = 6;
    int shadowAtlasSize = 4096;
    int shadowAtlasTileSize = 1024;
    int pointShadowPoolSize = 6;
    int pointShadowFaceSize = 1024;
};
