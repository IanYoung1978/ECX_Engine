#version 430 core
// Some drivers require the following
precision highp float;

layout (location = 0) uniform sampler2D fontAtlas;
uniform vec4 textColour;

in vec2 VSTexCoord;
out vec4 colour;

void main()
{
	// Atlas is single-channel (glyph coverage) - alpha comes from the glyph, colour is the
	// caller's chosen text colour.
	float coverage = texture(fontAtlas, VSTexCoord).r;
	colour = vec4(textColour.rgb, textColour.a * coverage);
}
