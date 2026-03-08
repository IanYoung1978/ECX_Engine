#version 410
in vec3 localPos;
uniform samplerCube skybox;
uniform samplerCube targetSkybox;
uniform float blendFactor;
uniform bool hasTarget;
out vec4 fragColour;

void main()
{
    vec3 colour = texture(skybox, localPos).rgb;
    if (hasTarget) {
        vec3 targetColour = texture(targetSkybox, localPos).rgb;
        colour = mix(colour, targetColour, blendFactor);
    }
    fragColour = vec4(colour, 1.0);
}