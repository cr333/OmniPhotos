#pragma once

#include <string>


class WindowInfo
{
public:
	WindowInfo() = default;
	WindowInfo(int _width, int _height);

	// dimensions
	int height = -1;
	int width = -1;

	// position
	int posX = 100;
	int posY = 100;

	void print();
};
