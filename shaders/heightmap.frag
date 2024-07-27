#version 410 core

in float Height;

out vec4 FragColor;

void main() {
    float heightColor = (Height + 16) / 64.f;

    FragColor = vec4(heightColor, heightColor, heightColor, 1.f);
}