#include "Loader.hpp"

#include "Core/Camera.hpp"
#include "Utils/Logger.hpp"

#include <vector>


Loader::Loader(std::vector<Camera*>* _cameras)
{
	cameras = _cameras;
}


Eigen::Vector2i Loader::getImageDims()
{
	if (cameras == nullptr)
	{
		LOG(WARNING) << "No cameras set in Loader.";
		return Eigen::Vector2i(0, 0);
	}

	// Load first image to get its dimensions.
	Camera* firstCamera = cameras->at(0);
	if (firstCamera->getImage().empty())
		firstCamera->loadImageWithOpenCV();

	cv::Mat firstImage = firstCamera->getImage();
	return Eigen::Vector2i(firstImage.cols, firstImage.rows);
}
