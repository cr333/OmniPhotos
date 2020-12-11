#include "GLFormats.hpp"
#include "Utils/Logger.hpp"

using namespace std;


GLuint getGLFormat(const string& in)
{
	// clang-format off
	if (in == "GL_BGR")  return GL_BGR;
	if (in == "GL_BGRA") return GL_BGRA;

	if (in == "GL_RED")  return GL_RED;

	if (in == "GL_RG")   return GL_RG;
	if (in == "GL_RGB")  return GL_RGB;
	if (in == "GL_RGBA") return GL_RGBA;
	
	if (in == "GL_RGBA_DXT1") return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	if (in == "GL_RGBA_DXT5") return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	// clang-format on

	LOG(WARNING) << "Couldn't find GL constant for the image format for the string: " << in;
	return 0;
}


GLenum getGLType(const string& in)
{
	// clang-format off
	if (in == "GL_UNSIGNED_BYTE")  return GL_UNSIGNED_BYTE;
	if (in == "GL_FLOAT")          return GL_FLOAT;
	if (in == "GL_UNSIGNED_SHORT") return GL_UNSIGNED_SHORT;
	// clang-format on

	LOG(WARNING) << "Couldn't find GL constant for the image type for the string: " << in;
	return 0;
}


GLuint getGLInternalFormat(const string& in)
{
	// clang-format off
	if (in == "GL_RGBA32F") return GL_RGBA32F;
	if (in == "GL_RGBA16F") return GL_RGBA16F;
	if (in == "GL_RGBA8")   return GL_RGBA8;

	if (in == "GL_RGB32F")  return GL_RGB32F;
	if (in == "GL_RGB16F")  return GL_RGB16F;
	if (in == "GL_RGB8")    return GL_RGB8;

	if (in == "GL_RG16F")   return GL_RG16F;

	if (in == "GL_R32F")    return GL_R32F;
	if (in == "GL_R16F")    return GL_R16F;
	if (in == "GL_R8")      return GL_R8;
	
	if (in == "GL_RGBA_DXT1") return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	if (in == "GL_RGBA_DXT5") return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	// clang-format on

	LOG(WARNING) << "Couldn't find GL constant for the image internal format for the string: " << in;
	return 0;
}


int getChannelsFromInternalFormat(const string& internalFormat)
{
	if (internalFormat == "GL_RGBA32F" || internalFormat == "GL_RGBA16F" || internalFormat == "GL_RGBA8")
		return 4;

	if (internalFormat == "GL_RGB32F" || internalFormat == "GL_RGB16F" || internalFormat == "GL_RGB8")
		return 3;

	LOG(WARNING) << "Couldn't find number of channels for GL internal format " << internalFormat;
	return -1;
}


int getChannelsFromInternalFormat(GLint internalFormat)
{
	if (internalFormat == GL_RGBA32F || internalFormat == GL_RGBA16F || internalFormat == GL_RGBA8)
		return 4;

	if (internalFormat == GL_RGB32F || internalFormat == GL_RGB16F || internalFormat == GL_RGB8)
		return 3;

	LOG(WARNING) << "Couldn't find number of channels for GL internal format " << internalFormat;
	return -1;
}
