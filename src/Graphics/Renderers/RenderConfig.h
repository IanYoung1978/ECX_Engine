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
    // Multiplies DirLightShadowPBR.frag's read-side shadow bias (issue #97). The bias needed
    // to avoid self-shadowing acne scales with shadow-map texel density - roughly
    // (shadow box world-space width) / (atlas tile size in texels): a bigger box or a
    // smaller tile both mean each texel covers more world space, needing more bias to clear
    // the resulting depth-quantization error. The shader's own base bias constants were
    // tuned for this project's default scale (dirShadowDistance=50, shadowAtlasTileSize=
    // 1024) - a scene at a very different scale (much larger dirShadowDistance, or a
    // smaller atlas tile) should scale this up; a smaller/denser scene can likely scale it
    // down. 1.0 leaves the tuned base values unchanged.
    float dirShadowBiasScale = 1.0f;
    // Multiplies the intensity-weighted average colour of every active light in the scene
    // (see GL_Deferred_Renderer::updateLights) to get the flat ambient fill term applied in
    // emissivePass() - NOT an authored colour itself (issue #96: a scene with only, say,
    // red point lights and no directional light should get a reddish ambient, not
    // whatever fixed colour an author guessed at authoring time). This is just how strong
    // that derived fill is.
    float ambientScale = 0.15f;
};
