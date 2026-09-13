#version 410
in vec2 texCoord;
uniform sampler2D hdrBuffer;
uniform float exposure;
out vec4 fragColour;

vec3 ACESFilmic(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec3 hdrColour = texture(hdrBuffer, texCoord).rgb;
    vec3 mapped = ACESFilmic(hdrColour * exposure);
    // The only gamma encode anywhere in the renderer now - every lighting shader works
    // entirely in linear space (sRGB decode of albedo is automatic on texture sampling,
    // see TextureManager::finalizeTexture's isSRGB parameter) and this is the single choke
    // point everything passes through right before hitting the screen, after tonemapping.
    mapped = pow(mapped, vec3(1.0 / 2.2));
    fragColour = vec4(mapped, 1.0);
}