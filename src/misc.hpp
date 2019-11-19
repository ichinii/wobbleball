#pragma once

#include <ostream>
#include <glm/glm.hpp>
#include <chrono>

template <typename CharT, typename Traits, glm::length_t L, typename T, glm::qualifier Q>
inline
auto& operator << (std::basic_ostream<CharT, Traits>& os, glm::vec<L, T, Q>& vec)
{
	if (L == 0) return os << "()";
	os << '(' << vec[0];
	for (glm::length_t i = 1; i < L; ++i) os << '|' << vec[i];
	return os << ')';
}

template <typename CharT, typename Traits, typename Rep, typename Period>
inline
auto& operator << (std::basic_ostream<CharT, Traits>& os, std::chrono::duration<Rep, Period> duration)
{
	return os << duration.count();
}
