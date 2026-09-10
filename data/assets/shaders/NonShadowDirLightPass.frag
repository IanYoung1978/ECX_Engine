#version 430 core
precision highp float;

// Replaces lightpass.frag's role for directional lights with CastsShadow=false - real
// Cook-Torrance/GGX PBR instead of the old Blinn-Phong path, drawn one light per additive
// full-screen-quad pass (matching how DirLightShadowPBR.frag's shadow-casting counterpart
// is already invoked in GL_Deferred_Renderer::lightPass), just with no shadow-map sampling
// - visibility is implicitly 1.0. sRGB decode of AlbedoMap is automatic (the G-buffer's
// albedo texture is written from an sRGB-internal-format source - see
// TextureManager::finalizeTexture's isSRGB parameter), and gamma encode happens exactly
// once, centrally, in hdr_tonemap.frag - not here.

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D AlbedoMap;
layout (location = 3) uniform sampler2D PBRMap;
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

in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

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
	float roughness,
	float metal,
	float ao)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metal);
	vec3 H = normalize(Vdirection + Ldirection);
	float distance = length(Ldirection);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = Lcolour * (attenuation * Lintensity);

	float NDF = DistributionGGX(normal, H, roughness);
	float G   = GeometrySmith(normal, Vdirection, Ldirection, roughness);
	vec3 F    = fresnelSchlick(max(dot(H, Vdirection), 0.0), F0);

	vec3 nominator    = NDF * G * F;
	float denominator = 4 * max(dot(normal, Vdirection), 0.0) * max(dot(normal, Ldirection), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metal;

	// Strict clamp for specular (unchanged) - a highlight should never appear on a face
	// pointed away from the light, so this stays a hard cutoff at the true terminator.
	float NdotL = max(dot(normal, Ldirection), 0.0);
	// Half-Lambert wrap for diffuse only: a single hard directional light with no bounce/
	// fill light and a flat, low ambient floor makes the ordinary max(dot(N,L),0) cutoff
	// read as a sharp near-black band wherever a curved surface's normal crosses the
	// terminator within a few screen pixels - exactly what a rolling-hill ridge does, and
	// exactly what this was mistaken for a mesh/normal bug earlier (both G-buffers were
	// confirmed clean there). This remaps dot(N,L) from [-1,1] to [0,1] instead of clamping
	// it, so the diffuse term fades out smoothly through the terminator rather than
	// snapping straight to the ambient floor - a standard, deliberately non-physical fix
	// (raising the ambient floor alone only made the band less dark, not less sharp).
	float wrapNdotL = clamp(dot(normal, Ldirection) * 0.5 + 0.5, 0.0, 1.0);
	vec3 Lo = kD * albedo / PI * radiance * wrapNdotL + specular * radiance * NdotL;
	return Lo;
}

void main()
{
	vec4 pcolour 		= texture(positionMap, indata.VSTexCoord).rgba;
	if (pcolour.a == 0.0) discard;
	vec4 ncolour 		= texture(normalMap, indata.VSTexCoord).rgba;
	vec3 dcolour 		= texture(AlbedoMap, indata.VSTexCoord).rgb;
	vec3 pbr 			= texture(PBRMap, indata.VSTexCoord).rgb;
	vec3 vToEye 		= WSCamPos - pcolour.xyz;
	vToEye 				= normalize(vToEye);

	colour = vec4(computeLight(
		-dirLight.direction.xyz,
		vToEye,
		dirLight.colour.rgb,
		dcolour,
		ncolour.rgb, dirLight.intensity,
		pbr.r,
		pbr.g,
		pbr.b
	), 1.0);
}
