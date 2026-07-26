#version 430 core
// Some drivers require the following
precision highp float;

// Mip-chain bloom upsample step (see BloomChain.h for the overall design). Standard 3x3 tent
// filter over the smaller (coarser) source level, additively blended onto the destination
// level's existing content via GL blend state (GL_ONE, GL_ONE) - not blended in-shader.

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

	vec3 result = texture(sourceMap, uv).rgb * 4.0;
	result += texture(sourceMap, uv + vec2(-texelSize.x, 0.0)).rgb * 2.0;
	result += texture(sourceMap, uv + vec2( texelSize.x, 0.0)).rgb * 2.0;
	result += texture(sourceMap, uv + vec2(0.0, -texelSize.y)).rgb * 2.0;
	result += texture(sourceMap, uv + vec2(0.0,  texelSize.y)).rgb * 2.0;
	result += texture(sourceMap, uv + vec2(-texelSize.x, -texelSize.y)).rgb;
	result += texture(sourceMap, uv + vec2( texelSize.x, -texelSize.y)).rgb;
	result += texture(sourceMap, uv + vec2(-texelSize.x,  texelSize.y)).rgb;
	result += texture(sourceMap, uv + vec2( texelSize.x,  texelSize.y)).rgb;
	result /= 16.0;

	FragColor = vec4(result, 1.0);
}
