#version 410
layout(location = 0) in vec3 inPosition;

uniform mat4 ModelTransform;
uniform mat4 ViewTransform;
uniform mat4 ProjTransform;

void main()
{
    gl_Position = ProjTransform * ViewTransform * ModelTransform * vec4(inPosition, 1.0);
}