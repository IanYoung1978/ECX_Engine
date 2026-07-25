#pragma once

struct RenderConfig
{
    float exposure = 0.75f;
    int shadowAtlasSize = 4096;
    int shadowAtlasTileSize = 1024;
    int pointShadowPoolSize = 6;
    int pointShadowFaceSize = 1024;
};
