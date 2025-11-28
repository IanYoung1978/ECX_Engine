#version 430 core
// Some drivers require the following
precision highp float;
layout (location = 0)in vec3 MSVertex;
layout (location = 0) uniform mat4 ModelTransform;
layout (location = 1) uniform mat4 ViewTransform;
layout (location = 2) uniform mat4 ProjTransform;
void main()
{
	gl_Position = ProjTransform * ViewTransform * ModelTransform * vec4(MSVertex,1.0);
}