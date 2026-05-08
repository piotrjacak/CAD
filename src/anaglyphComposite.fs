#version 400 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D leftEyeTex;
uniform sampler2D rightEyeTex;

void main()
{
    vec3 L = texture(leftEyeTex,  texCoord).rgb;
    vec3 R = texture(rightEyeTex, texCoord).rgb;

    // Lewe oko  - czerwony
    // Prawe oko - zielony i niebieski (cyjan)
    fragColor = vec4(L.r, R.g, R.b, 1.0);
}
