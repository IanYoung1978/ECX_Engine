#version 430 core
// Some drivers require the following
precision highp float;

// Unit quad (0,0)-(1,1) in, scaled/offset in pixel space by the caller per-draw - lets one
// small VAO serve every UI background panel regardless of size/position.
layout (location = 0) in vec2 MSVertex;

uniform mat4 Projection;
uniform vec2 offset;
uniform vec2 scale;

void main()
{
	vec2 pos = MSVertex * scale + offset;
	gl_Position = Projection * vec4(pos, 0.0, 1.0);
}
