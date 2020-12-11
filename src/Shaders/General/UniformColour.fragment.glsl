#version 410 core

uniform vec3 inColor;
out vec4 color;

void main()
{
	// color = vec4(1.0, 0.0, 1.0, 1.0); // Debug: magenta
	color = vec4(inColor, 1.0);
}
