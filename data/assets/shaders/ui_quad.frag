#version 430 core
// Some drivers require the following
precision highp float;

uniform vec4 colour;
out vec4 FragColor;

void main()
{
	FragColor = colour;
}
