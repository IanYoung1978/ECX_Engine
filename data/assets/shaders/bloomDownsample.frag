#version 430 core
// Some drivers require the following
precision highp float;

// Mip-chain bloom downsample step (see BloomChain.h for the overall design). 4-tap box filter,
// sampling the larger source texture at the 4 texel centres surrounding the destination's UV -
// the standard cheap, alias-resistant downsample filter (better than a naive single bilinear
// tap, simpler than a full 13-tap variant).

layout (location = 0) uniform sampler2D sourceMap;
out vec4 FragColor;

in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(sourceMap, 0));
	vec2 uv = indata.VSTexCoord;

	vec3 result = vec3(0.0);
	result += texture(sourceMap, uv + vec2(-0.5, -0.5) * texelSize).rgb;
	result += texture(sourceMap, uv + vec2( 0.5, -0.5) * texelSize).rgb;
	result += texture(sourceMap, uv + vec2(-0.5,  0.5) * texelSize).rgb;
	result += texture(sourceMap, uv + vec2( 0.5,  0.5) * texelSize).rgb;
	result *= 0.25;

	FragColor = vec4(result, 1.0);
}
