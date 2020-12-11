#include "TextureLoader.hpp"

#include "Utils/ErrorChecking.hpp"


TextureLoader::TextureLoader(ImageLoader* _imageLoader)
{
	imageLoader = _imageLoader;
	setCameras(_imageLoader->getCameras());
}


TextureLoader::TextureLoader(ImageLoader* _imageLoader, FlowLoader* _flowLoader) :
    TextureLoader(_imageLoader)
{
	flowLoader = _flowLoader;
}


TextureLoader::~TextureLoader()
{
	if (imageLoader)
	{
		delete imageLoader;
		imageLoader = nullptr;
	}

	if (flowLoader)
	{
		delete flowLoader;
		flowLoader = nullptr;
	}
}


void TextureLoader::setCameras(std::vector<Camera*>* _cameras)
{
	cameras = _cameras;
	imageLoader->setCameras(cameras);
}


GLTexture* TextureLoader::getProjectionMatrixTexture()
{
	return imageLoader->getProjectionMatrixTexture();
}


GLTexture* TextureLoader::getImageTexture()
{
	return imageLoader->getImageTexture();
}


GLTexture* TextureLoader::getPosViewTexture()
{
	return imageLoader->getPosViewTexture();
}


bool TextureLoader::loadImages()
{
	imageLoader->setCameras(cameras);
	return imageLoader->loadImages();
}


GLTexture* TextureLoader::getForwardFlowTexture()
{
	return flowLoader->getForwardFlowsTexture();
}


GLTexture* TextureLoader::getBackwardFlowTexture()
{
	return flowLoader->getBackwardFlowsTexture();
}


bool TextureLoader::load()
{
	bool loadOK = true;
	if (imageLoader)
		loadOK &= imageLoader->loadImages();
	return loadOK;
}


void TextureLoader::releaseGPUMemory()
{
	if (imageLoader) imageLoader->releaseGPUMemory();
	if (flowLoader) flowLoader->releaseGPUMemory();
}


void TextureLoader::releaseCPUMemory()
{
	if (imageLoader) imageLoader->releaseCPUMemory();
	if (flowLoader) flowLoader->releaseCPUMemory();
}


void TextureLoader::initTextures()
{
	if (imageLoader) imageLoader->initTextures();
	if (flowLoader) flowLoader->initTextures();
}


void TextureLoader::loadTextures()
{
	if (imageLoader) imageLoader->loadTextures();
	if (flowLoader) flowLoader->loadTextures();
}


void TextureLoader::fillTextures()
{
	if (imageLoader) imageLoader->fillTextures();
	if (flowLoader) flowLoader->fillTextures();

	initialized = true;
}
