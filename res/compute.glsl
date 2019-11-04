#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D output_image;

uniform float elapsed_time;
uniform float delta_time;
uniform ivec2 mouse_coord;

float plane(vec3 p, vec3 n)
{
	return length(p) * dot(n, normalize(p));
}

float sphere(vec3 p)
{
	return length(p);
}

float cube(vec3 p)
{
	p = abs(p);
	return max(p.x, max(p.y, p.z));
}

float scene(vec3 p)
{
	float p0 = plane(p + vec3(0, 0, 0), normalize(vec3(0, 1, -.5))) + 3.;
	float s0 = sphere(p - vec3(-1, 0, 5)) - .5;
	float c0 = cube(p - vec3(1, 0, 5)) - .5;

	return min(p0, min(s0, c0));
}

bool march(vec3 ro, vec3 rd, out vec3 p, out float n)
{
	p = rd;
	float lo = 0.;

	for (int i = 0; i < 64; ++i) {
		float l = scene(p);

		p += l * rd;
		if (l < .001) {
			n = i / 64.;
			return true;
		}

		lo += l;
		if (lo > 100.)
			return false;
	}

	return false;
}

vec3 normal(vec3 p)
{
	float l = scene(p);
	vec2 e = vec2(0, .001);

	return normalize(
		l - vec3(
			scene(p - e.yxx),
			scene(p - e.xyx),
			scene(p - e.xxy)
		)
	);
}

void main() {
	vec2 output_size = vec2(imageSize(output_image));
  vec2 output_coord = gl_GlobalInvocationID.xy;
	if (output_coord.x >= output_size.x || output_coord.y >= output_size.y) return;

	vec2 uv = (output_coord - output_size * .5) / output_size.y;
  vec2 m = vec2(mouse_coord - output_size * .5) / output_size.y;
	vec3 c = vec3(0);

	vec3 ro = vec3(0);
	vec3 rd = normalize(vec3(uv.xy, 1));

	float n;
	if (march(ro, rd, ro, n)) {
		c += smoothstep(1., 0., n);
	}

	imageStore(output_image, ivec2(output_coord), vec4(c, 1));
}
