#version 410
in vec3 localPos;
uniform sampler2D equirectangularMap;
out vec4 fragColour;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = sampleSphericalMap(normalize(localPos));
    vec3 colour = texture(equirectangularMap, uv).rgb;
    fragColour = vec4(colour, 1.0);
}