#version 410 core

uniform int numberOfPoints;
flat in int vertex_id;
out vec4 color;


void main()
{
	color = vec4(vec3(gl_PrimitiveID / (numberOfPoints - 1.0)), 1.0);
}
