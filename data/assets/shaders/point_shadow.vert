#version 430 core
precision highp float;

layout (location = 0) in vec3 MSVertex;

uniform mat4 ModelTransform;
uniform mat4 ViewTransform;
uniform mat4 ProjTransform;

out vec3 FragPos;

void main()
{
    vec4 worldPos = ModelTransform * vec4(MSVertex, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = ProjTransform * ViewTransform * worldPos;
}