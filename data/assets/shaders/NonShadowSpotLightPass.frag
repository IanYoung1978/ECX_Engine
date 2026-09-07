#version 430 core
precision highp float;

// Replaces lightpass.frag's role for spot lights with CastsShadow=false - real
// Cook-Torrance/GGX PBR instead of the old Blinn-Phong path, drawn one light per additive
// full-screen-quad pass (matching how SpotlightShadowPBR.frag's shadow-casting counterpart
// is already invoked), just with no shadow-map sampling - visibility is implicitly 1.0.
// sRGB decode of AlbedoMap is automatic; gamma encode happens once, centrally, in
// hdr_tonemap.frag.

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D AlbedoMap;
layout (location = 3) uniform sampler2D PBRMap;
const float PI = 3.14159265359;

struct SpotLightData
{
	vec4 colour;
	vec4 position;
	vec4 attenuation;
	float intensity;
	vec3 padding;
	vec4 direction;
	float cutoffAngle;
};

uniform SpotLightData spotLight;
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
	vec3 pbr
)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, pbr.g);
	vec3 H = normalize(Vdirection + Ldirection);
	float distance = length(Ldirection);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = Lcolour * (attenuation * Lintensity);

	float NDF = DistributionGGX(normal, H, pbr.r);
	float G   = GeometrySmith(normal, Vdirection, Ldirection, pbr.r);
	vec3 F    = fresnelSchlick(max(dot(H, Vdirection), 0.0), F0);

	vec3 nominator    = NDF * G * F;
	float denominator = 4 * max(dot(normal, Vdirection), 0.0) * max(dot(normal, Ldirection), 0.0) + 0.001;
	vec3 specular = nominator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - pbr.g;

	float NdotL = max(dot(normal, Ldirection), 0.0);
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
	return Lo;
}

void main()
{
	vec4 pcolour 		= texture(positionMap, indata.VSTexCoord).rgba;
	if (pcolour.a == 0.0) discard;
	vec4 ncolour 		= texture(normalMap, indata.VSTexCoord).rgba;
	vec4 albedo 		= texture(AlbedoMap, indata.VSTexCoord);
	vec3 pbr 			= texture(PBRMap, indata.VSTexCoord).rgb;
	vec3 vToEye 		= normalize(WSCamPos - pcolour.xyz);
	vec3 outColour 		= vec3(0.0,0.0,0.0);

	vec3 ltf 			= spotLight.position.xyz - pcolour.rgb;
	float cos_cur_angle = dot(normalize(-ltf),spotLight.direction.xyz);
	if (cos_cur_angle > cos(spotLight.cutoffAngle))
	{
		vec3 fragCol 	= computeLight(
							-spotLight.direction.xyz,
							vToEye,
							spotLight.colour.rgb,
							albedo.rgb,
							ncolour.rgb,
							spotLight.intensity,
							pbr
						);
		outColour = fragCol * clamp((cos_cur_angle-spotLight.cutoffAngle)/(1.0-spotLight.cutoffAngle), 0.0, 1.0);
	}
	colour = vec4(outColour,1);
}
