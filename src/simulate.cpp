#include <iostream>
#include "simulate.hpp"
#include "misc.hpp"
#include <optional>
#include <glm/gtx/rotate_vector.hpp>
#include "keyboard.hpp"

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
	using glm::max;

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
	// for (int i = 0; i < 16; ++i) {
	// 	ps.push_back(glm::vec3(((std::rand() % 100) / 50.f - 1.f) * 3.f, 0.f, ((std::rand() % 100) / 50.f - 1.f) * 3.f));
	// 	vs.push_back(glm::vec3(0, -1.f, 0));
	// }

	ps.push_back(glm::vec3(-3, 0, 0));
	ms.push_back(glm::vec3(0, 1, 0));
	ws.push_back(1.f);
}

std::optional<glm::vec3> intersectSphereScene(glm::vec3 pos, float r, float elapsed = 0.f)
{
	const int sr = 8;
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
	const float speed = 1.f;
	float elapsed = elapsed_time.count() / 1000.f;
	assert(ps.size() == ms.size() && ps.size() == ws.size());
	delta_time = delta_time / 2;
	float r = .2f;

	for (auto i = 0ul; i < ps.size(); ++i) {
		auto& pos = ps[i];
		auto& mom = ms[i];
		auto& weight = ws[i];
		weight += (s_keyboard[GLFW_KEY_S] ? (weight * 3.f) : (1.f - weight) * 10.f) * (delta_time.count() / 1000.f);
		weight = glm::min(weight, 10.f);
		std::cout << weight << std::endl;

		mom += glm::vec3(0, -weight, 0) * (delta_time.count() / 1000.f * speed);
		pos += (mom / weight) * (delta_time.count() / 1000.f * speed);

		if (auto opos = intersectSphereScene(pos, r, elapsed); opos) {
			auto apos = *opos; // absolute collision pos
			auto lpos = apos - pos + glm::vec3(0, .001f, 0);	// local collision pos
			auto lposl = glm::length(lpos);
			auto lposd = glm::normalize(lpos);
			auto n = normal(apos + lposd * .01f); // normal of the scene
			auto cosa = glm::dot(mom, n);

			// adjust position
			pos += lposd * (lposl - r);

			// moving towards collision point
			if (cosa < 0.f) {

				// bounce
				auto bounce = glm::reflect(mom, n);

				mom = glm::mix(mom - cosa * n, bounce, s_keyboard[GLFW_KEY_E] ? 1. : .5);
			}

			// jump
			if (s_keyboard[GLFW_KEY_W])
				mom += n * weight;
		}
	}
}
