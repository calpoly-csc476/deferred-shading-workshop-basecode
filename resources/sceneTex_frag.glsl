#version 330 core

uniform sampler2D Texture0;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;

uniform sampler2D materialTex;
uniform vec3 lightDir;

in vec3 fNormal;
in vec2 fTexCoord;

void main()
{
	float ambient = 0.3;
	float diffuse = 0.7;

	outColor = texture(materialTex, fTexCoord).rgb;
	outNormal = normalize(fNormal) * 0.5 + 0.5;
}
