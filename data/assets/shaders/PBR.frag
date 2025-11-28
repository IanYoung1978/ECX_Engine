#version 430 core
// Some drivers require the following
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

in xferBlock
{
	vec3 TSViewPos; 
	vec3 TSVertex;
	vec3 WSVertex;
	vec3 WSNormal;
	vec2 TexCoord;
	mat3 TBN;
}indata;

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
	const float minLayers = 8.0;
	const float maxLayers = 64.0;
	vec2 P = viewDir.xy / viewDir.z * 0.05 + 0.001; // scale + bias
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir))); 
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;

	P.y *= -1.0; // flip the y coordinate
	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = texCoords;
	float currentDepthMapValue = 1.0 - texture(heightMap, texCoords).r;

	//Loop until the point on the height map is hit
	while(currentLayerDepth < currentDepthMapValue)
	{
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		// get depthmap value at current texture coordinates
		currentDepthMapValue = 1.0 - texture(heightMap, currentTexCoords).r;  
		// get depth of next layer
		currentLayerDepth += layerDepth;  
	}
	
	// Apply occlusion
	// get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    return finalTexCoords;
}

void main()
{

	vec3 pbr = vec3(1.0,1.0,1.0);
	vec2 txc = indata.TexCoord;
	position = vec4(indata.WSVertex,1.0);
	if (hasMaterial == 1)
	{
		vec2 txc = parallaxMapping(indata.TexCoord,normalize(indata.TSViewPos - indata.TSVertex));

		if(txc.x > 1.01 || txc.y > 1.01 || txc.x < -0.01 || txc.y < -0.01)
			discard;
		albedo = vec4(texture(colourMap,txc).rgb,1.0);
		vec3 normal_rgb = texture(normalMap,txc).rgb;
		normal_rgb = normal_rgb * 2.0 - 1.0;
		normal_rgb = normalize(indata.TBN * normal_rgb);
		pbr.r = texture(smoothnessMap,txc).r;
		pbr.g = texture(metalMap,txc).r;
		pbr.b = texture(AOMap,txc).r;
		PBR = vec4(pbr,1.0);
		nColour = vec4(normal_rgb,1.0);
		glow = vec4(texture(glowMap,txc).rgb,1.0);
	}
	else
	{
		albedo = incolour;
		nColour = vec4(indata.WSNormal,1.0);
		PBR = vec4(1.0,1.0,1.0,1.0);
	}
}