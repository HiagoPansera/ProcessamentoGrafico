#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform float tx;
uniform float ty;
uniform float offsetX;
uniform float tileW;

void main()
{
    gl_Position = vec4(aPos.x + tx, aPos.y + ty, 0.0, 1.0);
    TexCoord = vec2(aTexCoord.x * tileW + offsetX, aTexCoord.y);
}