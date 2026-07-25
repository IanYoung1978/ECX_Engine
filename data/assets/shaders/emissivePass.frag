#version 430 core
// Some drivers require the following
precision highp float;

// Adds the G-buffer's emissive/glow contribution exactly once per frame, independent of
// how many lights are active. Previously every per-light lighting shader (base lightpass,
// each shadow-casting light's quad, each receivesShadow-exempt pass) added glowMap
// redundantly - with N active shadow-casting lights, emissive glow was N times too bright.

layout (location = 0) uniform sampler2D glowMap;
uniform float intensity;
out vec4 colour;

in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

void main()
{
	colour = vec4(texture(glowMap, indata.VSTexCoord).rgb * intensity, 1.0);
}
