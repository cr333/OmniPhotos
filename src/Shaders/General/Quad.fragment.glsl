#version 410 core

uniform sampler2D renderedTexture;

in vec2 UV;

out vec4 color;


void main()
{	
	color = texture(renderedTexture, UV);
}
