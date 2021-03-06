#version 330 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 fNormal;

void main()
{
	/* First model transforms */
	gl_Position = P * V * M * vec4(vertPos.xyz, 1.0);

	/* the normal in worldspace */
	fNormal = (M * vec4(normalize(vertNor), 0.0)).xyz;
}
