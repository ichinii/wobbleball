#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D output_image;

uniform float elapsed_time;
uniform float delta_time;
uniform ivec2 mouse_coord;
uniform mat4 view;

mat2 rot(float a)
{
	float s = sin(a);
	float c = cos(a);
	return mat2(c, -s, s, c);
}

float smin(float a, float b, float k)
{
	float h = clamp(.5 + .5 * (b - a) / k, 0., 1.);
	return mix(b, a, h) - k * h * (1. - h);
}

float plane(vec3 p, vec3 n)
{
	return dot(p, n);
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

float roundcube(vec3 p)
{
	p = abs(p);
	return -smin(-p.x, smin(-p.y, -p.z, .2), .2);
}

float capsule(vec3 p, vec3 a, vec3 b)
{
	vec3 ab = b - a;
	vec3 ap = p - a;

	float t = dot(ab, ap) / dot(ab, ab);
	t = clamp(t, 0., 1.);

	vec3 c = a + t * ab;

	return length(p - c);
}

float torus(vec3 p, float r)
{
	float l = length(p.xz) - r;
	return length(vec2(l, p.y));
}

float cylinder(vec3 p, vec3 a, vec3 b, float r)
{
	vec3 ab = b - a;
	vec3 ap = p - a;
	float t = dot(ab, ap) / dot(ab, ab);
	vec3 c = a + t * ab;

	float x = length(p - c) - r;
	float y = (abs(t - .5) - .5) * length(ab);
	float e = length(max(vec2(x, y), 0.));
	float i = min(max(x, y), 0.);

	return e + i;
}

float scene(vec3 p)
{
	float s0 = cube(p) - .03;
	float p0 = plane(p - vec3(0, -1, 0), vec3(0, 1, 0));

	float ca0 = capsule(p, vec3(-1, -1, -2), vec3(1, 1, -2)) - .3;
	float ca1 = capsule(p, vec3(-1, 1, -2), vec3(1, -1, -2)) - .3;
	float x = smin(ca0, ca1, .1);

	float t0 = torus(p - vec3(.3, -.5, .3), .3) - .1;
	float cy0 = cylinder(p, vec3(-1, -.2, -.2), vec3(-1, .2, .2), .5);

	return min(min(s0, -smin(-p0, x - .2, .05)), min(min(x, t0), cy0));
}

bool march(vec3 ro, vec3 rd, out vec3 p, out float n)
{
	p = ro;
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
	uv *= 1.;
  vec2 m = vec2(mouse_coord - output_size * .5) / output_size.y;
	vec3 c = vec3(0);

	vec3 ro = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
	vec3 rd = (inverse(view) * vec4(normalize(vec3(uv.xy, -1)), 0)).xyz;
	/* vec3 rd = normalize(vec3(uv.xy, 1)); */

	float n;
	if (march(ro, rd, ro, n)) {
		c.r += smoothstep(1., 0., n);
		c.b += max(0., dot(normal(ro), -rd));
	}

	imageStore(output_image, ivec2(output_coord), vec4(c, 1));
}
