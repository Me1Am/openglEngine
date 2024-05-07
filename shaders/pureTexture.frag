#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D diffuseTexture1;

void main() {
	FragColor = texture(diffuseTexture1, TexCoord);
}