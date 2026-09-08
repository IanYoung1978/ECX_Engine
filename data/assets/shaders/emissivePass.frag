#version 430 core
// Some drivers require the following
precision highp float;

// Adds the G-buffer's emissive/glow contribution exactly once per frame, independent of
// how many lights are active. Previously every per-light lighting shader (base lightpass,
// each shadow-casting light's quad, each receivesShadow-exempt pass) added glowMap
// redundantly - with N active shadow-casting lights, emissive glow was N times too bright.
//
// The flat ambient term below has to live here for the exact same reason: this renderer
// has no real indirect/IBL lighting, just a flat ambientColour*albedo*AO term to give AO
// something to actually darken - if that were added inside each per-light shader instead
// (as every other lighting term is), it would be re-added once per active light via the
// additive blending those passes use, N times too bright for the same reason glow was.

layout (location = 0) uniform sampler2D glowMap;
layout (location = 1) uniform sampler2D AlbedoMap;
layout (location = 2) uniform sampler2D PBRMap;
uniform float intensity;
uniform vec3 ambientColour;
out vec4 colour;

in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} indata;

void main()
{
	vec3 glow = texture(glowMap, indata.VSTexCoord).rgb * intensity;
	vec3 albedo = texture(AlbedoMap, indata.VSTexCoord).rgb;
	float ao = texture(PBRMap, indata.VSTexCoord).b;
	colour = vec4(glow + ambientColour * albedo * ao, 1.0);
}
