#version 410 core

layout(location = 0) in vec3 vertexPosition_modelspace;

// Combined model-view-projection matrix in OpenGL coordinate system.
uniform mat4 MVP;
uniform mat4 pose;

out vec4 worldPos;


void main()
{	
	vec3 pos = vertexPosition_modelspace;
	worldPos = pose * vec4(pos, 1.0);
	gl_Position = MVP * worldPos;
}
