#version 330 core

uniform sampler2D Texture0;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;

uniform vec3 materialColor;
uniform vec3 lightDir;

in vec3 fNormal;

void main()
{
	outColor = materialColor;
	outNormal = normalize(fNormal) * 0.5 + 0.5;
}
