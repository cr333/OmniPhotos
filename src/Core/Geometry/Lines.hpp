#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Shape.hpp"


/** Class to represent lines. */
class Lines : public Shape, public GLRenderable
{
public:
	/** For uniform lines. */
	Lines(const std::vector<float>& _vertices);

	/** For coloured lines. */
	Lines(const std::vector<float>& _vertices, const std::vector<float>& _colours);

	virtual ~Lines() = default;

	void createRenderModel(const std::string& _name);
};
