#pragma once

#include "Core/GL/GLTexture.hpp"
#include "Core/Loaders/Loader.hpp"


class ImageLoader : public Loader
{
public:
	ImageLoader(std::vector<Camera*>* _cameras);
	~ImageLoader();


	// TODO: We should probably remove this function in favour of (load|init|fill)Textures.
	bool loadImages();

	// Reads data from disk to CPU memory.
	void loadTextures() override;

	// Creates GPU textures.
	void initTextures() override;

	// Uploads data from CPU memory to GPU memory. Fills images, projection matrices and flows.
	void fillTextures() override;


	// Releases all resources in CPU memory.
	void releaseCPUMemory() override;

	// Releases all resources in GPU memory.
	void releaseGPUMemory() override;


	// Getters
	inline GLTexture* getImageTexture() const { return imageTexture; }
	inline GLTexture* getProjectionMatrixTexture() const { return projectionMatrixTexture; }
	inline GLTexture* getPosViewTexture() const { return posViewTexture; }

	// The texture format used on the GPU:
	// GL_RGB: uncompressed textures
	// DXT1: DXT1 texture compression (RGB)
	// DXT5: DXT5 texture compression (RGBA)
	std::string imageTextureFormat = "GL_RGB";

private:
	// Stores the texture formats supported by the current graphics card.
	static std::vector<std::string> supportedTextureFormats;

	// Checks if the texture compression format is supported and images are compatible.
	bool checkTextureFormat();

	// used by "fillCameraTextureOpenCV"
	void uploadImageToOpenGLTexture(cv::Mat& img, GLTexture* texture, int layer);

	// Fills images and camera projection matrices
	void setPosViewTex(int layer);

	void updateSinglePosViewTex(int layer);
	void updatePosViewTex();
	void fillCameraTextureOpenCV(Camera* cam, int layer);

	void fillProjectionTexture(Camera* cam, int layer);
	void updateProjectionTex();


	GLTexture* imageTexture = nullptr;
	GLTexture* projectionMatrixTexture = nullptr;
	GLTexture* posViewTexture = nullptr;
};
