#version 410 core

uniform sampler2D diffuse;
in vec2 v2TexCoord;
out vec4 color;

void main()
{
   color = texture(diffuse, v2TexCoord);
}
