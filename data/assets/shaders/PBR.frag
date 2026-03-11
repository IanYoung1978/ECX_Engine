#version 430 core
precision highp float;
layout (location = 1) uniform sampler2D colourMap;
layout (location = 2) uniform sampler2D smoothnessMap;
layout (location = 3) uniform sampler2D normalMap;
layout (location = 4) uniform sampler2D heightMap;
layout (location = 5) uniform sampler2D metalMap;
layout (location = 6) uniform sampler2D AOMap;
layout (location = 7) uniform sampler2D glowMap;
uniform vec4 incolour;
layout (location = 0) out vec4 position;
layout (location = 1) out vec4 nColour;
layout (location = 2) out vec4 albedo;
layout (location = 3) out vec4 PBR;
layout (location = 4) out vec4 glow;
uniform int hasMaterial;
uniform float parallaxScale;
uniform float parallaxBias;
in xferBlock
{
    vec3 TSViewPos; 
    vec3 TSVertex;
    vec3 WSVertex;
    vec3 WSNormal;
    vec2 TexCoord;
    mat3 TBN;
} indata;

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 64.0;

    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    // Fixed: clamp division, scale only (no bias on displacement)
    vec2 P = clamp(viewDir.xy / max(viewDir.z, 0.001), -2.0, 2.0) * parallaxScale;
    P.y *= -1.0;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    // Fixed: bias applied to depth comparison only
    float currentDepthMapValue = 1.0 - texture(heightMap, currentTexCoords).r + parallaxBias;

    // Fixed: capped for loop instead of while
    for (int i = 0; i < int(maxLayers); i++)
    {
        if (currentLayerDepth >= currentDepthMapValue) break;
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - texture(heightMap, currentTexCoords).r + 0.001;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - texture(heightMap, prevTexCoords).r + parallaxBias - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

void main()
{
    vec3 pbr = vec3(1.0, 1.0, 1.0);

    if (hasMaterial == 1)
    {
        vec2 txc = parallaxMapping(indata.TexCoord, normalize(indata.TSViewPos - indata.TSVertex));

        if (txc.x > 1.01 || txc.y > 1.01 || txc.x < -0.01 || txc.y < -0.01)
            discard;

        position = vec4(indata.WSVertex, 1.0);
        albedo = vec4(texture(colourMap, txc).rgb, 1.0);

        vec3 normal_rgb = texture(normalMap, txc).rgb;
        normal_rgb = normal_rgb * 2.0 - 1.0;
        normal_rgb = normalize(indata.TBN * normal_rgb);
        nColour = vec4(normal_rgb, 1.0);

        pbr.r = texture(smoothnessMap, txc).r;
        pbr.g = texture(metalMap, txc).r;
        pbr.b = texture(AOMap, txc).r;
        PBR = vec4(pbr, 1.0);
        glow = vec4(texture(glowMap, txc).rgb, 1.0);
    }
    else
    {
        position = vec4(indata.WSVertex, 1.0);
        albedo = incolour;
        nColour = vec4(indata.WSNormal, 1.0);
        PBR = vec4(1.0, 1.0, 1.0, 1.0);
        glow = vec4(0.0, 0.0, 0.0, 1.0);
    }
}