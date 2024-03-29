#include <iostream>
#include <numeric>
#include <algorithm>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shader.hpp"
#include "simulate.hpp"
#include "misc.hpp"
#include "keyboard.hpp"
#include "watcher.hpp"

using namespace std::chrono_literals;

int main()
{
	std::srand(std::time(0));

	if (!glfwInit()) return 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	auto window_size = glm::ivec2{1280, 720};
	auto window = glfwCreateWindow(window_size.x, window_size.y, "glsl", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	keyboardInit(window);

	if (glewInit() != GLEW_OK) return 0;
	glClearColor(.2, .1, 0, 1);

	// std::this_thread::sleep_for(1s);
	auto display_program = createProgram({{GL_VERTEX_SHADER, "res/vertex.glsl"}, {GL_FRAGMENT_SHADER, "res/fragment.glsl"}});
	auto compute_program = createProgram({{GL_COMPUTE_SHADER, "res/compute.glsl"}});

	auto compute_shader_watcher = Watcher("res/compute.glsl", [&] () {
		if (compute_program > 0)
			glDeleteProgram(compute_program);
		compute_program = createProgram({{GL_COMPUTE_SHADER, "res/compute.glsl"}});
	});

	glUseProgram(compute_program);

	auto frame_tex_size = glm::uvec2(window_size);
	GLuint frame_tex_out;
	glGenTextures(1, &frame_tex_out);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_tex_out);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, frame_tex_size.x, frame_tex_size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, frame_tex_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	
	glUseProgram(display_program);
	enum { vertex_position, vertex_uv };
	GLuint vao;
	GLuint vbos[2];
	glCreateVertexArrays(1, &vao);
	glGenBuffers(2, vbos);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(vertex_position);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vertex_position]);
	glm::vec2 vertex_positions[] = { {-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof (vertex_positions), vertex_positions, GL_STATIC_DRAW);
	glVertexAttribPointer(vertex_position, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(vertex_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vertex_uv]);
	glm::vec2 vertex_uvs[] = { {0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof (vertex_uvs), vertex_uvs, GL_STATIC_DRAW);
	glVertexAttribPointer(vertex_uv, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindVertexArray(0);
	glUseProgram(0);

	Simulate simulate;

	using clock = std::chrono::steady_clock;
	auto start_time = clock::now();
	auto elapsed_time = 0ms;
	auto fps_print_time = elapsed_time;
	float timescale = 1.f;

	auto frames = 0ul;
	while (!glfwWindowShouldClose(window)) {
		auto cur_time = clock::now();
		auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(cur_time - (start_time + elapsed_time));
		elapsed_time += delta_time;

		compute_shader_watcher.update();

		auto elapsed_time_ms = elapsed_time;
		auto delta_time_ms = delta_time;
		elapsed_time_ms *= timescale;
		delta_time_ms *= timescale;

		glfwPollEvents();
		glfwGetWindowSize(window, &window_size.x, &window_size.y);
		glViewport(0, 0, window_size.x, window_size.y);
		frame_tex_size = glm::uvec2(window_size);
		glBindTexture(GL_TEXTURE_2D, frame_tex_out);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, frame_tex_size.x, frame_tex_size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindImageTexture(0, frame_tex_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		simulate.update(elapsed_time_ms, delta_time_ms);

		glm::dvec2 mouse;
		glfwGetCursorPos(window, &mouse.x, &mouse.y);
		mouse.y = window_size.y - mouse.y + 1;

		float x = (mouse.x - window_size.x * .5f) / window_size.y;
		float y = (mouse.y - window_size.y * .5f) / window_size.y;
		x *= glm::pi<float>();
		y *= glm::pi<float>();
		auto camera_center_pos = glm::vec3(3, 3, 3);
		glm::mat4 view = glm::lookAt(camera_center_pos + 5.f * glm::vec3{
			cos(y) * sin(x),
			sin(y),
			cos(y) * cos(x)
		}, camera_center_pos, {0, 1, 0});

		{ // launch compute shaders and draw to image
			glUseProgram(compute_program);
			glUniform1f(glGetUniformLocation(compute_program, "elapsed_time"), elapsed_time_ms.count() / 1000.f);
			glUniform1f(glGetUniformLocation(compute_program, "delta_time"), delta_time_ms.count() / 1000.f);
			glUniform2i(glGetUniformLocation(compute_program, "mouse_coord"), mouse.x, mouse.y);
			glUniformMatrix4fv(glGetUniformLocation(compute_program, "view"), 1, GL_FALSE, &view[0][0]);
			auto players_size = std::min(16ul, simulate.ps.size());
			glUniform1i(glGetUniformLocation(compute_program, "players_size"), players_size);
			glUniform3fv(glGetUniformLocation(compute_program, "players_pos"), players_size, reinterpret_cast<float*>(&simulate.ps[0]));
			glDispatchCompute(frame_tex_size.x / 16 + 1, frame_tex_size.y / 16 + 1, 1);
		}

		// make sure writing to image has finished before read
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		{ // present image to screen
			glUseProgram(display_program);
			glUniform2i(glGetUniformLocation(display_program, "tex_size"), frame_tex_size.x, frame_tex_size.y);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindVertexArray(vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frame_tex_out);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
			glfwSwapBuffers(window);
			glBindVertexArray(0);
		}

		++frames;
		if (fps_print_time.count() + 1000 * timescale < elapsed_time.count()) {
			fps_print_time += elapsed_time - fps_print_time;
			std::cout << frames << " fps" << std::endl;
			frames = 0;
		}

		glfwSetWindowShouldClose(window, s_keyboard[GLFW_KEY_Q]);
	}

	glfwTerminate();

	return 0;
}
