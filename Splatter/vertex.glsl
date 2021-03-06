#version 330 core

// Per-vertex inputs
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float size;

// Matrices we'll need
layout(std140) uniform constants
{
	mat4 mv_matrix;
	mat4 view_matrix;
	mat4 proj_matrix;
};

// Inputs from vertex shader
out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} vs_out;

// Position of light
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);
uniform float modelLength = 1.0f;
uniform float modelScale = 1.0f;

void main(void)
{
	// Calculate view-space coordinate
	vec4 P = mv_matrix * position;

	// Calculate normal in view-space
	vs_out.N = mat3(mv_matrix) * normal;

	// Calculate light vector
	vs_out.L = - P.xyz;

	// Calculate view vector
	vs_out.V = -P.xyz;

	// Calculate the clip-space position of each vertex
	gl_Position = proj_matrix * P;
	gl_PointSize = modelScale * size;
}
