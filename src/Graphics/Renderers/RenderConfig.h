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
    // Multiplies DirLightShadowPBR.frag's read-side shadow bias. The bias needed scales with shadow-map
    // texel size, roughly (shadow box width) / (atlas tile size); the shader's base constants are tuned
    // for dirShadowDistance=50 and a 1024 tile, so scale up for larger scenes or smaller tiles. 1.0
    // leaves the base values unchanged.
    float dirShadowBiasScale = 1.0f;
    // Strength of the ambient fill: multiplies the intensity-weighted average colour of the active
    // lights (see GL_Deferred_Renderer::updateLights), applied in emissivePass().
    float ambientScale = 0.15f;
};
