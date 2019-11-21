#version 450 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D output_image;

uniform float elapsed_time;
uniform float delta_time;
uniform ivec2 mouse_coord;
uniform mat4 view;

uniform int players_size;
uniform vec3 players_pos[16];

#define light_pos vec3(8, 8, 8)

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

float smax(float a, float b, float k)
{
	return -smin(-a, -b, k);
}

float plane(vec3 p, vec3 n)
{
	return dot(p, n);
}

float sphere(vec3 p)
{
	return length(p);
}

float cube(vec3 p, float r)
{
	p = abs(p);
	return length(p - min(p, r));
}

float roundcube(vec3 p, vec2 r)
{
	float c = cube(p, r.x - r.y);
	return c - r.y;
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

float torus(vec3 p, vec2 r)
{
	float l = length(p.xz) - r.x;
	return length(vec2(l, p.y)) - r.y;
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
	float p0 = plane(
		p - vec3(0, sin(p.x + elapsed_time) * .5 + sin(p.z * .5 + elapsed_time * .3) * .5, 0),
		normalize(vec3(sin(elapsed_time * .35) * .1, 1, sin(elapsed_time) * .1)));
	float s1 = sphere(p - vec3(0, 0, 0));
	float ground = smax(p0, -s1, 10.);

	float c0 = roundcube(p - vec3(0, -2., 0), vec2(1., .2));
	ground = smin(ground, c0, 2.);

	float t0 = torus(p - vec3(0, -1, 0), vec2(5., 1.3));
	ground = smin(ground, t0, 1.);

	float s2 = sphere(p - vec3(0, 1, 0)) - 7.5;
	float p1 = plane(p - vec3(0, 2, 0), vec3(0, -1, 0));
	ground = smin(ground, max(-s2, -p1), 2.);

	float players = 100.;
	for (int i = 0; i < min(16, players_size); ++i) {
		players = min(players, sphere(p - players_pos[i]));
	}

	return min(players - .2, ground);
}

bool march(vec3 ro, vec3 rd, out vec3 p, out float it)
{
	p = ro;
	float lo = 0.;

	for (int i = 0; i < 64*10; ++i) {
		float l = scene(p);

		p += l * rd;
		if (l < .001) {
			it = i / 64.;
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

	float it;
	vec3 p;
	if (march(ro, rd, p, it)) {
		vec3 n = normal(p);
		c.r += smoothstep(1., 0., it);
		c.b += max(0., dot(n, -rd));
		
		vec3 ld = normalize(light_pos - p);
		float cosa = dot(n, ld);
		/* c *= max(.2, cosa); */

		ro = p + n * .005;
		rd = ld;
		if (march(ro, rd, p, it) && length(p - ro) < length(light_pos - ro))
			c *= .4;
	}

	imageStore(output_image, ivec2(output_coord), vec4(pow(c, vec3(.45)), 1));
}
