#version 430 core
// Some drivers require the following
precision highp float;


layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D AlbedoMap;
layout (location = 3) uniform sampler2D PBRMap;
layout (location = 5) uniform sampler2DShadow shadowMap;
const float PI = 3.14159265359;
struct LightData
{
	vec4 colour;
	vec4 position;
	vec4 attenuation;
	float intensity;
};
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

struct DirLightData
{
	vec4 colour;
	float intensity;
	vec4 direction;
};

uniform mat4 ShadowTransform;
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
	// calculate radiance
	vec3 H = normalize(Vdirection + Ldirection);
	float distance = length(Ldirection);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = Lcolour * (attenuation * Lintensity);

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(normal, H, smoothness);   
	float G   = GeometrySmith(normal, Vdirection, Ldirection, smoothness);      
	vec3 F    = fresnelSchlick(max(dot(H, Vdirection), 0.0), F0);
           
	vec3 nominator    = NDF * G * F; 
	float denominator = 4 * max(dot(normal, Vdirection), 0.0) * max(dot(normal, Ldirection), 0.0) + 0.001; // 0.001 to prevent divide by zero.
	vec3 specular = nominator / denominator;
        
	// kS is equal to Fresnel
	vec3 kS = F;
	// for energy conservation, the diffuse and specular light can't
	// be above 1.0 (unless the surface emits light); to preserve this
	// relationship the diffuse component (kD) should equal 1.0 - kS.
	vec3 kD = vec3(1.0) - kS;
	// multiply kD by the inverse metalness such that only non-metals 
	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// have no diffuse light).
	kD *= 1.0 - metal;	  

	// scale light by NdotL
	float NdotL = max(dot(normal, Ldirection), 0.0);        
	// reflectance equation
	// add to outgoing radiance Lo
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	vec3 color	= (albedo * 0.2)  + Lo;  
	return Lo;
}

float computeOcclusion(vec4 shadowCoords)
{
	vec3 coord 			= vec3(shadowCoords.xyz/shadowCoords.w);
	float depth 		= texture( shadowMap, vec3(coord.xy,coord.z));
	if ( depth < coord.z - 0.001) // bias = 0.001
		return 0.2;
	return 1.0;
}

void main()
{
	vec4 pcolour 		= texture(positionMap, indata.VSTexCoord).rgba;
	if (pcolour.a == 0.0) discard;
	vec4 ncolour 		= texture(normalMap, indata.VSTexCoord).rgba;
	vec3 dcolour 		= pow(texture(AlbedoMap, indata.VSTexCoord).rgb,vec3(2.2));
	vec3 pbr 			= texture(PBRMap, indata.VSTexCoord).rgb;
	vec4 shadowCoord 	= ShadowTransform * pcolour;
	float visibility 	= computeOcclusion( shadowCoord );
	vec3 vToEye 		= WSCamPos - pcolour.xyz;
	vToEye 				= normalize(vToEye);
	vec3 outColour 		= vec3(0.0,0.0,0.0);

	outColour 			= computeLight(
							-dirLight.direction.xyz,
							vToEye,
							dirLight.colour.rgb,
							dcolour, 
							ncolour.rgb,dirLight.intensity,
							pbr.r,
							pbr.g,
							pbr.b
						);
	colour = vec4(pow(visibility*outColour, vec3(1.0/2.2)), 1.0);
}