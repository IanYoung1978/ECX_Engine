#version 410
layout(location = 0) in vec3 MSVertex;

uniform mat4 ViewTransform;
uniform mat4 ProjTransform;

out vec3 localPos;

void main()
{
    localPos = MSVertex;
    gl_Position = ProjTransform * ViewTransform * vec4(MSVertex, 1.0);
}