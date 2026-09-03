#version 430 core
// Some drivers require the following
precision highp float;
layout (location = 0)in vec3 MSVertex;
// Locations 10-12, not 0-2: this vertex shader is also linked against the "Exempt" lighting
// shaders (ShadowExempt*LightPass.frag), whose G-buffer samplers already claim locations
// 0-3 - explicit locations must be unique across the whole linked program, not just within
// one stage, so sharing 0-2 with those silently failed to link (see Shader::loadShader's
// GL_LINK_STATUS check). Uniforms are still set by name from C++, so the actual numbers
// here don't matter as long as they're free.
layout (location = 10) uniform mat4 ModelTransform;
layout (location = 11) uniform mat4 ViewTransform;
layout (location = 12) uniform mat4 ProjTransform;
void main()
{
	gl_Position = ProjTransform * ViewTransform * ModelTransform * vec4(MSVertex,1.0);
}
