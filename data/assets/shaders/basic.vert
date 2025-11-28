#version 430 core
// Some drivers require the following
precision highp float;
layout (location = 0)in vec3 MSVertex;
layout (location = 1)in vec3 MSNormal;
layout (location = 2)in vec3 MSTangent;
layout (location = 3)in vec3 MSBiTangent;
layout (location = 4)in vec2 MSTexCoord;

uniform mat4 ModelTransform;
uniform mat4 ViewTransform;
uniform mat4 ProjTransform;
uniform vec3 camPos;
out xferBlock
{
	vec3 TSViewPos;
	vec3 TSVertex;
	vec3 WSVertex;
	vec3 WSNormal;
	vec2 TexCoord;
	mat3 TBN;
} outdata;


void main()
{
	mat3 normalMat = transpose(inverse(mat3(ModelTransform)));
	vec3 T = normalize(vec3(normalMat * MSTangent));
	vec3 B = normalize(vec3(normalMat * MSBiTangent));
	vec3 N = normalize(vec3(normalMat * MSNormal));
	mat3 tbn = mat3(T, B, N);
	outdata.TBN = tbn;
	vec4 vert = ModelTransform * vec4(MSVertex,1.0);
	outdata.WSVertex = vert.xyz;
	outdata.WSNormal = normalize(mat3(ModelTransform) * MSNormal);
	outdata.TexCoord = MSTexCoord;
	outdata.TSViewPos = transpose(tbn) * camPos;
	outdata.TSVertex = transpose(tbn) * vert.xyz;
	gl_Position = ProjTransform * ViewTransform * vert;
}