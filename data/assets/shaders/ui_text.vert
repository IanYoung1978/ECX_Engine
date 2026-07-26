#version 430 core
// Some drivers require the following
precision highp float;

layout (location = 0) in vec2 MSVertex;
layout (location = 4) in vec2 MSTexCoord;

uniform mat4 Projection;

out vec2 VSTexCoord;

void main()
{
	VSTexCoord = MSTexCoord;
	gl_Position = Projection * vec4(MSVertex, 0.0, 1.0);
}
