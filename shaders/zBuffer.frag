#version 330 core

out vec4 FragColor;

float far = 100.0;	// The far plane
float near = 0.1;	// The near plane

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Convert to normalized device coordinate

    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {	
	FragColor = vec4(vec3(linearizeDepth(gl_FragCoord.z) / far), 1.0);
}