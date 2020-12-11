#version 410 core

// All drawing operations share this shader as input.
layout(location = 0) in vec3 vertexPosition_modelspace;

uniform mat4 MVP;


void main()
{
	gl_Position = MVP * vec4(vertexPosition_modelspace, 1.0);
}
