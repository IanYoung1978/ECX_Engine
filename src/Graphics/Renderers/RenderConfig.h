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
    // How far along the camera's view the directional-light shadow frustum reaches -
    // deliberately less than the camera's own draw distance, to keep shadow-map texel
    // density reasonable (see GL_Deferred_Renderer::shadowDirPass).
    float dirShadowDistance = 50.0f;
};
