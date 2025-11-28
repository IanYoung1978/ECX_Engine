#version 430 core
// Some drivers require the following
precision highp float;
#define MAX_NUM_POINT_LIGHTS 128

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D colourMap;
layout (location = 3) uniform sampler2D specularMap;
layout (location = 4) uniform sampler2D glowMap;
layout (location = 5) uniform sampler2DShadow shadowMap;

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
uniform SpotLightData spotLight;
out vec4 colour;
uniform vec3 WSCamPos;
in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

vec3 computeLight(
	vec3 Ldirection, 
	vec3 Vdirection, 
	vec3 Lcolour,
	vec3 dcolour,
	vec3 normal, 
	float Lintensity, 
	float specular)
{
	vec3 diffCol = dcolour * max(0.0,dot(normalize(normal),normalize(-Ldirection)));
	vec3 reflectVec = normalize(reflect(Ldirection,normal));
	float specFactor = max(dot(reflectVec,Vdirection),0);
	float specPow = pow(specFactor,255.0)*specular;
	vec3 specCol = Lcolour * specPow;
	return (diffCol+specCol) * Lintensity;
}

float computeOcclusion(vec4 shadowCoords)
{
	vec3 coord = vec3(shadowCoords.xyz/shadowCoords.w);
	float depth = texture( shadowMap, vec3(coord.xy,coord.z));
	if ( depth < coord.z - 0.001)
		return 0.2;
	return 1.0;
}

void main()
{
	vec4 pcolour = texture(positionMap, indata.VSTexCoord).rgba;
	vec4 ncolour = texture(normalMap, indata.VSTexCoord).rgba;
	vec4 dcolour = texture(colourMap, indata.VSTexCoord).rgba;
	vec4 scolour = texture(specularMap, indata.VSTexCoord).rgba;
	vec4 gcolour = texture(glowMap, indata.VSTexCoord).rgba;
	vec4 shadowCoords = ShadowTransform * pcolour;
	float visibility = computeOcclusion( shadowCoords );
	vec3 vToEye = WSCamPos - pcolour.xyz;
	vToEye = normalize(vToEye);
	vec3 outColour = vec3(0.0,0.0,0.0);

	vec3 ltf = spotLight.position.xyz - pcolour.rgb;
	float cos_cur_angle = dot(normalize(-ltf),spotLight.direction.xyz);
	if (cos_cur_angle > cos(spotLight.cutoffAngle))
	{
		vec3 fragCol = computeLight(
						spotLight.direction.xyz,
						vToEye,
						spotLight.colour.rgb,
						dcolour.rgb, 
						ncolour.rgb,
						spotLight.intensity,
						scolour.r
		);
		outColour= fragCol * clamp((cos_cur_angle-spotLight.cutoffAngle)/(1.0-spotLight.cutoffAngle), 0.0, 1.0);

	}
	colour = vec4(outColour,1.0);
	//colour = vec4(visibility*dcolour.rgb*outColour+gcolour.rgb,1);
	//colour = vec4(dcolour.rgb*outColour,1);
	//colour = vec4(depth,depth,depth,1.0);
	colour = ncolour;
	//colour = vec4(shadowCoords.z,shadowCoords.z,shadowCoords.z,1.0);

}