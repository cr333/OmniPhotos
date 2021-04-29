#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Shape.hpp"

/** class to generate lines */
class Lines : public Shape, public GLRenderable
{
public:
	// Uniform lines
	Lines(const std::vector<float>& _vertices);

	// Coloured lines
	Lines(const std::vector<float>& _vertices, const std::vector<float>& _colours);

	virtual ~Lines() = default;

	void createRenderModel(const std::string& _name);

private:
	//optional requirements:
	//1. colour_buffer
};
