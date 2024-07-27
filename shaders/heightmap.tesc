#version 410 core

layout (vertices = 4) out;

in vec2 TexCoord[];

out vec2 TextureCoord[];	// Output to (generator and) evaluation shader

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

	// Subdivide 16 times on each edge(0 controls entire quad)
	if(gl_InvocationID == 0){
		gl_TessLevelOuter[0] = 16;
		gl_TessLevelOuter[1] = 16;
		gl_TessLevelOuter[2] = 16;
		gl_TessLevelOuter[3] = 16;

		gl_TessLevelInner[0] = 16;
		gl_TessLevelInner[1] = 16;
	}
}