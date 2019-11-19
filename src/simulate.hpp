#pragma once

#include <chrono>
#include <vector>
#include <glm/glm.hpp>

class Simulate {
public:
	Simulate();
	void update(std::chrono::milliseconds, std::chrono::milliseconds);

	std::vector<glm::vec3> ps;
	std::vector<glm::vec3> vs;
};
