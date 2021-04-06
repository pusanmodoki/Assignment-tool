#pragma once
#include <functional>
#include <random>

//ランダム用クラス
class Random
{
public:
	inline Random(std::size_t max) : m_device(std::mt19937(std::random_device{}())),
		m_distribution(std::uniform_int_distribution<std::size_t>(0, max)) {}

	inline int operator ()()
	{
		return m_distribution(m_device);
	}

private:
	std::mt19937 m_device;
	std::uniform_int_distribution<std::size_t> m_distribution;
};