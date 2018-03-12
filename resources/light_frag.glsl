#version 330 core

uniform sampler2D Texture0;

layout(location = 0) out vec3 outColor;

// Light Properties
uniform vec3 lightColor;
uniform vec3 lightPosition;

// G-Buffer
uniform sampler2D sceneColorTex;
uniform sampler2D sceneNormalsTex;
uniform sampler2D sceneDepthTex;

uniform mat4 invP;
uniform mat4 invV;

vec3 reconstructWorldspacePosition(vec2 texCoords)
{
	float depth = texture(sceneDepthTex, texCoords).r;
	vec3 ndc = vec3(texCoords, depth) * 2.0 - vec3(1.0);
	vec4 view = invP * vec4(ndc, 1.0);
	view.xyz /= view.w;
	vec4 world = invV * vec4(view.xyz, 1.0);
	return world.xyz;
}

void main()
{
	const float ambient = 0.3;
	const float diffuse = 0.7;

	vec2 texCoord = gl_FragCoord.xy / vec2(textureSize(sceneDepthTex, 0));

	vec3 materialColor = texture(sceneColorTex, texCoord).rgb;
	vec3 normal = normalize(texture(sceneNormalsTex, texCoord).rgb * 2.0 - vec3(1.0));
	vec3 fragPos = reconstructWorldspacePosition(texCoord);

	vec3 light = normalize(lightPosition - fragPos);

	float lambert = clamp(dot(normal, light), 0.0, 1.0);

	outColor = (ambient + diffuse * lambert * lightColor) * materialColor;
}
