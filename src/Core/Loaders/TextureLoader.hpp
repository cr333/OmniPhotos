#pragma once

#include "Core/Loaders/FlowLoader.hpp"
#include "Core/Loaders/ImageLoader.hpp"

#include <GL/gl3w.h>


class TextureLoader
{
public:
	TextureLoader() = default;
	TextureLoader(ImageLoader* _imageLoader);
	TextureLoader(ImageLoader* _imageLoader, FlowLoader* _flowLoader);

	~TextureLoader();

	inline bool isInitialized() const { return initialized; }

	// Updates the cameras in all loaders.
	void setCameras(std::vector<Camera*>* _cameras);
	inline std::vector<Camera*>* getCameras() const { return cameras; }

	inline ImageLoader* getImageLoader() const { return imageLoader; }
	inline FlowLoader* getFlowLoader() const { return flowLoader; }

	// Image textures
	GLTexture* getImageTexture();

	// Viewpoint extrinsics
	GLTexture* getProjectionMatrixTexture();

	GLTexture* getPosViewTexture();


	// Reads all data from disk to CPU memory.
	void loadTextures();

	// Creates GPU textures.
	void initTextures();

	// Uploads all data from CPU memory to GPU memory.
	void fillTextures();

	
	// Releases all resources in CPU memory.
	void releaseCPUMemory();

	// Releases all resources in GPU memory.
	void releaseGPUMemory();


	// Load images from disk to CPU memory, and store them in Camera objects.
	bool loadImages();

	// Loads colour images and depth maps. Flow is loaded in fillTextures().
	bool load();

	// Get texture objects from the loaders.
	GLTexture* getForwardFlowTexture();
	GLTexture* getBackwardFlowTexture();


private:
	bool initialized = false;

	// Loads colour input images.
	ImageLoader* imageLoader = nullptr;

	// Loads optical flow between neighbouring pairs of images.
	FlowLoader* flowLoader = nullptr;

	// Set of cameras to load images/flows/depth maps for.
	std::vector<Camera*>* cameras = nullptr;
};
