#include <iostream>
#include "simulate.hpp"
#include "misc.hpp"
#include <optional>

using namespace glm;

float smin(float a, float b, float k)
{
	float h = glm::clamp(.5 + .5 * (b - a) / k, 0., 1.);
	return glm::mix(b, a, h) - k * h * (1. - h);
}

float smax(float a, float b, float k)
{
	return -smin(-a, -b, k);
}

float plane(glm::vec3 p, glm::vec3 n)
{
	return glm::dot(p, n);
}

float sphere(glm::vec3 p)
{
	return glm::length(p);
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

float torus(vec3 p, vec2 r)
{
	float l = length(glm::vec2(p.x, p.z)) - r.x;
	return length(vec2(l, p.y)) - r.y;
}

float scene(glm::vec3 p, float elapsed_time = 0.f, float r = 0.f)
{
	using glm::min;

	float p0 = plane(
		p - vec3(0, sin(p.x + elapsed_time) * .5 + sin(p.z * .5 + elapsed_time * .3) * .5, 0),
		normalize(vec3(sin(elapsed_time * .35) * .1, 1, sin(elapsed_time) * .1)));
	float s1 = sphere(p - vec3(0, 0, 0));
	float ground = smax(p0, -s1, 10.);

	float c0 = roundcube(p - vec3(0, -2., 0), vec2(1., .2));
	ground = smin(ground, c0, 2.);

	float t0 = torus(p - vec3(0, -1, 0), vec2(5., 1.3));
	ground = smin(ground, t0, 1.);

	return ground;
}

glm::vec3 normal(glm::vec3 p, float elapsed = 0.f)
{
	auto l = scene(p, elapsed);
	auto x = 0.f;
	auto y = .001f;

	return glm::normalize(
		l - glm::vec3(
			scene(p - glm::vec3{y, x, x}, elapsed),
			scene(p - glm::vec3{x, y, x}, elapsed),
			scene(p - glm::vec3{x, x, y}, elapsed)
		)
	);
}

Simulate::Simulate()
{
	// for (int i = 0; i < 4; ++i) {
	// 	ps.push_back(glm::vec3(((std::rand() % 100) / 50.f - 1.f) * 3.f, 0.f, ((std::rand() % 100) / 50.f - 1.f) * 3.f));
	// 	vs.push_back(glm::vec3(0, -1.f, 0));
	// }

	ps.push_back(glm::vec3(-3, 0, 0));
	vs.push_back(glm::vec3(0, -1, 0));
}

std::optional<glm::vec3> intersectSphereScene(glm::vec3 pos, float r, float elapsed = 0.f)
{
	const int sr = 16;
	float lmin = r;
	glm::vec3 pmin;
	for (int x = -sr; x < sr + 1; ++x) {
		for (int y = -sr; y < sr + 1; ++y) {
			for (int z = -sr; z < sr + 1; ++z) {
				auto p = pos + glm::vec3(x, y, z) / static_cast<float>(sr) * r;
				bool b = scene(p, elapsed) < 0.f;
				float l = glm::length(pos - p);
				if (b && l < lmin) {
					lmin = l;
					pmin = p;
				}
			}
		}
	}

	if (lmin < r)
		return pmin;
	return {};
}

void Simulate::update(std::chrono::milliseconds elapsed_time, std::chrono::milliseconds delta_time)
{
	float elapsed = elapsed_time.count() / 1000.f;
	assert(ps.size() == vs.size());
	delta_time = delta_time / 2;

	for (auto i = 0ul; i < ps.size(); ++i) {
		auto& vel = vs[i];
		auto& pos = ps[i];

		// vel += glm::vec3(0, -1, 0) * (delta_time.count() / 1000.f);
		vel = glm::normalize(glm::mix(vel, glm::vec3(0, -1, 0), .05f));
		pos += vel * (delta_time.count() / 1000.f);

		auto opos = intersectSphereScene(pos, .2f, elapsed);

		// std::cout << pos << " " << l << " " << n << std::endl;

		if (opos) {
			auto ipos = *opos;
			float b = glm::sign(scene(ipos, elapsed));
			glm::vec3 n = -b * normal(ipos, elapsed);
			
			auto nvel = glm::normalize(vel);
			float cosa = glm::dot(n, nvel);
			auto rvel = glm::mix(vel, glm::reflect(nvel, n), cosa * -.5 + .5);
			vel = glm::normalize(rvel) * glm::length(vel);
			// vel += n * .1f;
			pos += n * (.2f - glm::length(pos - ipos));
		}
	}
}
