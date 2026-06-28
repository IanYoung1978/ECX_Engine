#version 430 core
precision highp float;

in vec3 FragPos;

uniform vec3 LightPos;
uniform float FarPlane;

void main()
{
    float lightDistance = length(FragPos - LightPos);
    gl_FragDepth = lightDistance / FarPlane;
}