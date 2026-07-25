#version 430 core
// Some drivers require the following
precision highp float;

// Issue #28: redraws receivesShadow==false entities with full (unshadowed) lighting from
// this directional light. Trimmed copy of DirLightShadowPBR.frag with computeOcclusion/
// shadowMap removed - visibility is implicitly 1.0. Paired with shadow.vert (a bare MVP
// transform, no varyings), so sampling uses screen-space texelFetch instead of a
// full-screen-quad UV varying - correct here since this pass draws real mesh geometry.

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D AlbedoMap;
layout (location = 3) uniform sampler2D PBRMap;
layout (location = 4) uniform sampler2D glowMap;
const float PI = 3.14159265359;

struct DirLightData
{
	vec4 colour;
	float intensity;
	vec4 direction;
};

uniform DirLightData dirLight;
out vec4 colour;
uniform vec3 WSCamPos;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      		= roughness*roughness;
    float a2     		= a*a;
    float NdotH  		= max(dot(N, H), 0.0);
    float NdotH2 		= NdotH*NdotH;
	const float PI		= 3.14159265359;
    float num   		= a2;
    float denom 		= (NdotH2 * (a2 - 1.0) + 1.0);
    denom 				= PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r 			= (roughness + 1.0);
    float k 			= (r*r) / 8.0;

    float num   		= NdotV;
    float denom 		= NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV 		= max(dot(N, V), 0.0);
    float NdotL 		= max(dot(N, L), 0.0);
    float ggx2  		= GeometrySchlickGGX(NdotV, roughness);
    float ggx1  		= GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 computeLight(
	vec3 Ldirection,
	vec3 Vdirection,
	vec3 Lcolour,
	vec3 albedo,
	vec3 normal,
	float Lintensity,
	float smoothness,
	float metal,
	float ao)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo,metal);
	vec3 H = normalize(Vdirection + Ldirection);
	float distance = length(Ldirection);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = Lcolour * (attenuation * Lintensity);

	float NDF = DistributionGGX(normal, H, smoothness);
	float G   = GeometrySmith(normal, Vdirection, Ldirection, smoothness);
	vec3 F    = fresnelSchlick(max(dot(H, Vdirection), 0.0), F0);

	vec3 nominator    = NDF * G * F;
	float denominator = 4 * max(dot(normal, Vdirection), 0.0) * max(dot(normal, Ldirection), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metal;

	float NdotL = max(dot(normal, Ldirection), 0.0);
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
	return Lo;
}

void main()
{
	ivec2 px 			= ivec2(gl_FragCoord.xy);
	vec4 pcolour 		= texelFetch(positionMap, px, 0);
	if (pcolour.a == 0.0) discard;
	vec4 ncolour 		= texelFetch(normalMap, px, 0);
	vec3 dcolour 		= pow(texelFetch(AlbedoMap, px, 0).rgb, vec3(2.2));
	vec3 pbr 			= texelFetch(PBRMap, px, 0).rgb;
	vec4 gcolour 		= texelFetch(glowMap, px, 0);
	vec3 vToEye 		= normalize(WSCamPos - pcolour.xyz);

	vec3 outColour 		= computeLight(
							-dirLight.direction.xyz,
							vToEye,
							dirLight.colour.rgb,
							dcolour,
							ncolour.rgb, dirLight.intensity,
							pbr.r, pbr.g, pbr.b
						);
	colour = vec4(pow(outColour + gcolour.rgb, vec3(1.0/2.2)), 1.0);
}
