#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <map>
#include <iostream>

class Keyboard {
public:
	enum Action : unsigned char {
		DownFlag = 1,
		ChangedFlag = 2
	};

	bool& operator[] (int key)
	{
		auto it = m_keys.find(key);
		if (it == m_keys.end()) {
			auto [it, b] = m_keys.emplace(key, false);
			return it->second;
		}
		return it->second;
	}

private:
	std::map<int, bool> m_keys;
};

inline Keyboard s_keyboard;

inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	s_keyboard[key] = action != GLFW_RELEASE;
}

inline void keyboardInit(GLFWwindow* window)
{
	glfwSetKeyCallback(window, key_callback);
}
