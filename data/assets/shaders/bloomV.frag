#version 430 core
out vec4 FragColor;
  
in xferBlock
{
	vec3 VSVertex;
	vec2 VSTexCoord;
}indata;

layout (location = 0) uniform sampler2D glowMap;
  
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(glowMap, 0); // gets size of single texel
    vec3 result = texture(glowMap, indata.VSTexCoord).rgb * weight[0]; // current fragment's contribution
	for(int i = 1; i < 5; ++i)
	{
		result += texture(glowMap, indata.VSTexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		result += texture(glowMap, indata.VSTexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
	}
    FragColor = vec4(result, 1.0);
}