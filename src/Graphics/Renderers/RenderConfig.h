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
    // Multiplies the intensity-weighted average colour of every active light in the scene
    // (see GL_Deferred_Renderer::updateLights) to get the flat ambient fill term applied in
    // emissivePass() - NOT an authored colour itself (issue #96: a scene with only, say,
    // red point lights and no directional light should get a reddish ambient, not
    // whatever fixed colour an author guessed at authoring time). This is just how strong
    // that derived fill is.
    float ambientScale = 0.15f;
};
