#version 410 core

layout (quads, fractional_odd_spacing, cw) in;

in vec2 TextureCoord[];

uniform sampler2D heightmap;	// Heightmap texture
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float Height;	// Output to fragment shader

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    // Get each vertex from the quad's uvs
	vec2 t00 = TextureCoord[0];
	vec2 t01 = TextureCoord[1];
	vec2 t10 = TextureCoord[2];
	vec2 t11 = TextureCoord[3];

	// Bilinearly interpolate uvs across quad
	vec2 t0 = (t01 - t00) * u + t00;
	vec2 t1 = (t11 - t10) * u + t10;
	vec2 texCoord = (t1 - t0) * v + t0;

	// Lookup texel at quad coordinate for height(from texture) and scale + shift
	Height = texture(heightmap, texCoord).y * 64.f - 16.f;

	// Get each vertex from the quad's 3D position
	vec4 p00 = gl_in[0].gl_Position;
	vec4 p01 = gl_in[1].gl_Position;
	vec4 p10 = gl_in[2].gl_Position;
	vec4 p11 = gl_in[3].gl_Position;

	// Calculate the quad's normal vector
	vec4 uVec = p01 - p00;
	vec4 vVec = p10 - p00;
	vec4 normal = normalize(vec4(cross(vVec.xyz, uVec.xyz), 0));

	// Bilinearly interpolate position across quad
	vec4 p0 = (p01 - p00) * u + p00;
	vec4 p1 = (p11 - p10) * u + p10;
	vec4 p = (p1 - p0) * v + p0;

	// Displace point along normal, ie tessellate
	p += normal * Height;

	gl_Position = projection * view * model * p;	// Output quad point into clip space

}