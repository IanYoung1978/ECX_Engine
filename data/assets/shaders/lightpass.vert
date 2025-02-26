#version 430
// Some drivers require the following
precision highp float;
layout (location = 0)in vec3 MSVertex;
layout (location = 1)in vec3 MSNormal;
layout (location = 2)in vec3 MSTangent;
layout (location = 3)in vec3 MSBiTangent;
layout (location = 4)in vec2 MSTexCoord;

out xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
} outdata;

void main()
{
	outdata.VSVertex = MSVertex;
	outdata.VSTexCoord = MSTexCoord;
	gl_Position = vec4(MSVertex,1.0);
}