// This is a texture sampler.  It lets you sample textures!  The keyword
// "uniform" means constant - sort of.  The uniform variables are the same
// for all fragments in an object, but they can change in between objects.
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampled2D normalMap;

// Diffuse, ambient, and specular materials.  These are also uniform.
uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ka;
uniform float alpha;
uniform bool hasNormalMapping;

// These are values that OpenGL interpoates for us.  Note that some of these
// are repeated from the fragment shader.  That's because they're passed
// across.
varying vec2 texcoord;
varying vec3 normal;
varying vec3 eyePosition;

varying vec3 tangent;
varying vec3 bitangent;

void main() {

	// Normalize the normal, and calculate light vector and view vector
	// Note: this is doing a directional light, which is a little different
	// from what you did in Assignment 2.
	vec3 N = normalize(normal);

	if (hasNormalMapping) {
		mat3 normalMatrix = mat3(tangent, bitangent, normal);

		/* Sample the normal */
		N = texture2D(normalMap, texcoord).rgb;

		/* Decompress the normal */
		N = N * vec3(2.0, 2.0, 2.0) - vec3(1.0, 1.0, 1.0);

		/* Calculate the normal */
		N = normalMatrix * gl_NormalMatrix * N;
	}

	vec3 L = normalize(gl_LightSource[0].position.xyz);
	vec3 V = normalize(-eyePosition);

	// Calculate the diffuse color coefficient, and sample the diffuse texture
	float Rd = max(0.0, dot(L, N));
	vec3 Td = texture2D(diffuseMap, texcoord).rgb;
	vec3 diffuse = Rd * Kd * Td * gl_LightSource[0].diffuse.rgb;

	// Calculate the specular coefficient
	vec3 R = reflect(-L, N);
	float Rs = pow(max(0.0, dot(V, R)), alpha);
	vec3 Ts = texture2D(specularMap, texcoord).rgb;
	vec3 specular = Rs * Ks * Ts * gl_LightSource[0].specular.rgb;

	// Ambient is easy
	vec3 ambient = Ka * gl_LightSource[0].ambient.rgb;

	// This actually writes to the frame buffer

	gl_FragColor = vec4(diffuse + specular + ambient, 1);
}
