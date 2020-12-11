#include "WindowInfo.hpp"

#include "Utils/Logger.hpp"

#include <iostream>


WindowInfo::WindowInfo(int _width, int _height) :
    height(_height), width(_width)
{
}


void WindowInfo::print()
{
	VLOG(1) << "WindowInfo: " << width << " x " << height << " at (" << posX << ", " << posY << ")";
}
