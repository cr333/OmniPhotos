#version 410 core

in vec4 worldPos;
out vec4 color;


void main()
{
	color = vec4(worldPos.xyz, 1.0);	
}
