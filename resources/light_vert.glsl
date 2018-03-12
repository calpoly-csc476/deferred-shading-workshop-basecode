#version 330 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 fNormal;
out vec3 fWorldPos;

void main()
{
	vec4 Pos = V * M * vec4(vertPos, 1.0);
	fWorldPos = Pos.xyz;
	gl_Position = P * Pos;
	fNormal = (M * vec4(vertNor, 0.0)).xyz;
}
