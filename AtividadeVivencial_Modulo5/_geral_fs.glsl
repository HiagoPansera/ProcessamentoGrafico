#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform float weight;

void main()
{
    vec4 texColor = texture(ourTexture, TexCoord);

    if(texColor.a < 0.1)
        discard;
        
    FragColor = mix(texColor, vec4(0.2, 0.2, 1.0, 1.0), weight);
}