#pragma once

#include "3rdParty/Eigen.hpp"
#include "Core/Camera.hpp"

#include <vector>


class Loader
{
public:
	Loader() = default;
	Loader(std::vector<Camera*>* _cameras);


	// Reads data from disk to CPU memory.
	virtual void loadTextures() = 0;

	// Creates GPU textures.
	virtual void initTextures() = 0;

	// Uploads data from CPU memory to GPU memory.
	virtual void fillTextures() = 0;


	// Releases all resources in CPU memory.
	virtual void releaseCPUMemory() = 0;

	// Releases all resources in GPU memory.
	virtual void releaseGPUMemory() = 0;


	// Gets the list of associated cameras.
	std::vector<Camera*>* getCameras() const { return cameras; };

	// Sets the list of associated cameras.
	void setCameras(std::vector<Camera*>* _cameras) { cameras = _cameras; }


	// Returns the number of cameras/images.
	inline int getImageCount() const { return cameras ? (int)cameras->size() : 0; }

	// Returns the image dimensions (width, height).
	Eigen::Vector2i getImageDims();


protected:

	// Just a pointer to the cameras held in a CameraSetup.
	std::vector<Camera*>* cameras = nullptr;
};
