#include "ViewerGLProgram.hpp"

#include "Core/CameraSetup/CameraSetupDataset.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"
#include "Core/GL/GLProgram.hpp"
#include "Core/GL/GLRenderModel.hpp"


//TODO: use indices should be read from the rendermodel
ViewerGLProgram::ViewerGLProgram(
    GLRenderModel* _render_model,
    CameraSetupSettings* _settings,
    CameraSetupDataset* _dataset) :
    GLProgram("ViewerGLProgram", _render_model)
{
	settings = _settings;
	dataset = _dataset;
}


void ViewerGLProgram::passUniforms(CameraInfo& camInfo)
{
	GLProgram::passUniformsBase(camInfo.matrix);

	setUniform("desCamPos", camInfo.centre);
	setUniform("desCamView", camInfo.forward);
	setUniform("raysPerPixel", settings->raysPerPixel);

	setUniform("displayMode", settings->displayMode);
	setUniform("useOpticalFlow", settings->useOpticalFlow);
	setUniform("flowDownsampled", settings->downsampleFlow);
	setUniform("useEquirectCamera", settings->useEquirectCamera);
	setUniform("fadeNearBoundary", settings->fadeNearBoundary);

	setUniform("circleNormal", dataset->circle->getNormal());
	setUniform("circleRadius", dataset->circle->getRadius());
}


// TODO(TB): This doesn't render vertex buffer with indices.
void ViewerGLProgram::draw()
{
	preDraw();

	glPointSize(settings->pointSize);
	ErrorChecking::checkGLError();

	auto prim = getActiveRenderModel()->getPrimitive();

	//Used by deformed sphered, but not the sphere nor the cylinder. These two render GL_TRIANGLE_STRIPs without indices

	//TODO: Check for making member of Primitive
	if (prim->use_indices)
	{
		ErrorChecking::checkGLError();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->indexBuffer->gl_ID);
		ErrorChecking::checkGLError();
		glDrawElements(GL_TRIANGLES, prim->indexCount, GL_UNSIGNED_INT, 0);
		ErrorChecking::checkGLError();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		ErrorChecking::checkGLError();
		glDrawArrays(prim->getRenderingMode(), 0, prim->vertexCount);
		ErrorChecking::checkGLError();
	}

	postDraw();
}
