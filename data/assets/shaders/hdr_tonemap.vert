#version 410
layout(location = 0) in vec3 MSVertex;
layout(location = 4) in vec2 MSTexCoord;

out vec2 texCoord;

void main()
{
    texCoord = MSTexCoord;
    gl_Position = vec4(MSVertex, 1.0);
}