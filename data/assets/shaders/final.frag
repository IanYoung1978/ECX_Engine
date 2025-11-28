#version 430
// Some drivers require the following
precision highp float;
layout (location = 0) uniform sampler2D colourMap;

layout (location = 0) out vec4 colour;

in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
}indata;

void main()
{
	float val = texture(colourMap, indata.VSTexCoord).r;
	colour = vec4(val, val, val, 1.0);
}