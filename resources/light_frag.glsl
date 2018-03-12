#version 330 core

uniform sampler2D Texture0;

in vec3 fNormal;
in vec3 fWorldPos;

layout(location = 0) out vec3 outColor;

// Light Properties
uniform vec3 lightColor;
uniform vec3 lightPos;

// Camera Properties
uniform mat4 invP;
uniform mat4 invV;
uniform vec3 cameraPos;

float raySphereIntersect(vec3 r0, vec3 rd, vec3 s0, float sr)
{
	// https://gist.github.com/wwwtyro/beecc31d65d1004f5a9d
	// - r0: ray origin
	// - rd: normalized ray direction
	// - s0: sphere center
	// - sr: sphere radius
	// - Returns distance from r0 to first intersecion with sphere,
	//   or -1.0 if no intersection.
	float a = dot(rd, rd);
	vec3 s0_r0 = r0 - s0;
	float b = 2.0 * dot(rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sr * sr);
	if (b*b - 4.0*a*c < 0.0)
	{
		return -1.0;
	}
	return (-b - sqrt((b*b) - 4.0*a*c))/(2.0*a);
}

void main()
{
	const float ambient = 0.3;
	const float diffuse = 0.7;
	const float radius = 3.5;

	vec3 materialColor = vec3(1.0);
	vec3 normal = normalize(fNormal);
	vec3 fragPos = fWorldPos;

	vec3 light = vec3(2.0, 9.0, 1.0);
	float distance = length(light);
	float lambert = clamp(dot(normal, normalize(light)), 0.0, 1.0);
	float attenuation = 1.0 / pow(1 + distance / radius, 2.0);

	// Some raycast code that's only going to come into play when we implement deferred
	// Not essential, just to show us where the lights are !
	vec3 r0 = cameraPos;
	vec3 rd = normalize(fragPos - cameraPos);
	float raycastDist = raySphereIntersect(r0, rd, lightPos, 0.1);
	if (raycastDist > 0.0 && raycastDist < length(fragPos - cameraPos))
	{
		outColor = lightColor;
	}
	else
	{
		// Actual shading computation

		outColor = (ambient + diffuse * lambert) * lightColor;
	}
}
