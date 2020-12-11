#pragma once

#include "GLObject.hpp"

#include <vector>


class GLBuffer : public GLObject
{
public:
	GLBuffer(bool _arrayBuffer = true);
	GLBuffer(std::vector<GLBufferLayout>& _layouts, bool _arrayBuffer = true);
	GLBuffer(const GLBufferLayout& _layout, bool _arrayBuffer = true);

	virtual ~GLBuffer();


	void init() override;
	void preDraw();
	void postDraw();

	// These are important to define vertexAttribPointers etc.,
	// e.g. used to draw coloured points (point clouds) or coloured lines (coordinate axes).
	std::vector<GLBufferLayout> layouts;


private:
	//TODO: When is this ever false?
	//I don't think that we need to keep this explicitly
	bool arrayBuffer = true;
};
