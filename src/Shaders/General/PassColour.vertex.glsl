#version 410 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;

uniform mat4 MVP;
out vec4 vColor;


void main()
{
	gl_Position = MVP * vec4(vertexPosition_modelspace, 1.0);
	vColor = vec4(vertexColor, 1.0);
}
