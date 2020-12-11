#pragma once

#include <chrono>


class Timer
{
public:
	Timer() {}
	~Timer() {}

	void startTiming()
	{
		start = std::chrono::system_clock::now();
		running = true;
	}

	void endTiming()
	{
		end = std::chrono::system_clock::now();
	}

	std::chrono::duration<double> getElapsedTime()
	{
		if (running)
			endTiming();

		std::chrono::duration<double> elapsed_seconds = end - start;
		startTiming();

		return elapsed_seconds;
	}

	double getElapsedSeconds()
	{
		return getElapsedTime().count();
	}

private:
	std::chrono::time_point<std::chrono::system_clock> start;
	std::chrono::time_point<std::chrono::system_clock> end;
	bool running = false;
};


class ScopedTimer
{
public:
	ScopedTimer()
	{
		timer.startTiming();
	}

	double getElapsedSeconds()
	{
		return timer.getElapsedSeconds();
	}

private:
	Timer timer;
};
