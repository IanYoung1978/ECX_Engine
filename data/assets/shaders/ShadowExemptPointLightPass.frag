#version 430 core
precision highp float;

// Issue #28: redraws receivesShadow==false entities with full (unshadowed) lighting from
// this point light. Trimmed copy of PointShadowLightPass.frag with computeOcclusion/
// shadowMap removed - visibility is implicitly 1.0. Paired with shadow.vert (a bare MVP
// transform, no varyings), so sampling uses screen-space texelFetch instead of a
// full-screen-quad UV varying - correct here since this pass draws real mesh geometry.

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D AlbedoMap;
layout (location = 3) uniform sampler2D PBRMap;
layout (location = 4) uniform sampler2D glowMap;

const float PI = 3.14159265359;

struct LightData
{
    vec4 colour;
    vec4 position;
    vec4 attenuation;
    float intensity;
};

uniform LightData pointLight;
uniform vec3 WSCamPos;

out vec4 colour;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r     = (roughness + 1.0);
    float k     = (r * r) / 8.0;
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 computeLight(
    vec3 Ldirection,
    vec3 Vdirection,
    vec3 Lcolour,
    vec3 albedo,
    vec3 normal,
    float Lintensity,
    vec3 pbr)
{
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, pbr.g);
    vec3 H = normalize(Vdirection + Ldirection);
    float distance    = length(Ldirection);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = Lcolour * (attenuation * Lintensity);

    float NDF = DistributionGGX(normal, H, pbr.r);
    float G   = GeometrySmith(normal, Vdirection, Ldirection, pbr.r);
    vec3 F    = fresnelSchlick(max(dot(H, Vdirection), 0.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, Vdirection), 0.0) * max(dot(normal, Ldirection), 0.0) + 0.001;
    vec3 specular     = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - pbr.g;

    float NdotL = max(dot(normal, Ldirection), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    ivec2 px     = ivec2(gl_FragCoord.xy);
    vec4 pcolour = texelFetch(positionMap, px, 0);
    if (pcolour.a == 0.0) discard;
    vec4 ncolour = texelFetch(normalMap, px, 0);
    vec4 albedo  = texelFetch(AlbedoMap, px, 0);
    vec3 pbr     = texelFetch(PBRMap, px, 0).rgb;
    vec4 gcolour = texelFetch(glowMap, px, 0);

    vec3 vToEye = normalize(WSCamPos - pcolour.xyz);
    vec3 ltf    = pointLight.position.xyz - pcolour.xyz;

    vec3 fragCol = computeLight(
        normalize(ltf),
        vToEye,
        pointLight.colour.rgb,
        albedo.rgb,
        ncolour.rgb,
        pointLight.intensity,
        pbr);

    colour = vec4(fragCol + gcolour.rgb, 1.0);
}
