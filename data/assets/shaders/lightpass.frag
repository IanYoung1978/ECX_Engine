#version 430 core
// Some drivers require the following
precision highp float;
#define MAX_NUM_POINT_LIGHTS 128

layout (location = 0) uniform sampler2D positionMap;
layout (location = 1) uniform sampler2D normalMap;
layout (location = 2) uniform sampler2D colourMap;
layout (location = 3) uniform sampler2D PBRMap;
uniform int NumSpots;
uniform int NumPoints;

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
	
layout (std140) uniform PointLights
{
	LightData points[MAX_NUM_POINT_LIGHTS];
};

layout (std140) uniform SpotLights
{
	SpotLightData spots[MAX_NUM_POINT_LIGHTS];
};

uniform DirLightData dirLight;
out vec4 colour;
uniform vec3 WSCamPos;
in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

vec3 computeLightFragment(vec3 eyeVec, vec3 lightVec, vec3 normal, vec3 colour, vec3 attenuation, float intensity, float spec)
{
	//compute lighting fragment
	float dist = length(lightVec);
	lightVec = normalize(lightVec);

	vec3 diffCol = colour * max(0.0,dot(normalize(normal),lightVec));
	vec3 reflectVec = normalize(reflect(-lightVec,normal));
	float specFactor = max(dot(reflectVec,eyeVec),0);
	float specPow = pow(specFactor,spec*255.0);
	vec3 specCol = colour * specPow;
	float attn = 1.0/(attenuation.x + 
	attenuation.y * dist + 
	attenuation.z * dist * dist);
	vec3 fragcol = (diffCol+specCol)*intensity*attn;
	return fragcol;
}
void main()
{
    vec4 pcolour = texture(positionMap, indata.VSTexCoord).rgba;
    // No geometry written here � let skybox show through
    if (pcolour.a == 0.0) discard;
	vec4 ncolour = texture(normalMap, indata.VSTexCoord).rgba;
	vec4 dcolour = texture(colourMap, indata.VSTexCoord).rgba;
	vec4 scolour = texture(PBRMap, indata.VSTexCoord).rgba;
	vec3 vToEye = WSCamPos - pcolour.xyz;
	vToEye = normalize(vToEye);
	vec3 outColour = vec3(0.0,0.0,0.0);
	for (int i = 0; i < NumPoints; i++)
	{
		//compute lighting fragment
		vec3 ltf = points[i].position.xyz - pcolour.rgb;
		outColour+=computeLightFragment(vToEye, ltf, ncolour.rgb, points[i].colour.rgb, points[i].attenuation.xyz, points[i].intensity, scolour.r);
	}
	for (int i = 0; i < NumSpots; i++)
	{
		vec3 ltf = spots[i].position.xyz - pcolour.rgb;
		float cos_cur_angle = dot(normalize(-ltf),spots[i].direction.xyz);
		if (cos_cur_angle > cos(spots[i].cutoffAngle))
		{
			vec3 fragCol = computeLightFragment(vToEye, ltf, ncolour.rgb, spots[i].colour.rgb, spots[i].attenuation.xyz, spots[i].intensity, scolour.r);
			outColour+=fragCol * clamp((cos_cur_angle-spots[i].cutoffAngle)/(1.0-spots[i].cutoffAngle), 0.0, 1.0);
		}
	}

	vec3 diffCol = dirLight.colour.rgb * max(0.0,dot(normalize(ncolour.rgb),-dirLight.direction.xyz));
	vec3 reflectVec = normalize(reflect(dirLight.direction.xyz,ncolour.rgb));
	float specFactor = max(dot(reflectVec,vToEye),0);
	float specPow = pow(specFactor,scolour.r*255.0);
	vec3 specCol = (dirLight.colour.rgb * diffCol)* specPow;
	outColour+=(diffCol+specCol)*dirLight.intensity;
		
	colour = vec4(dcolour.rgb*outColour.rgb,1.0);
}