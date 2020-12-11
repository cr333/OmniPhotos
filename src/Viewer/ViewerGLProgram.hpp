#pragma once

#include "Core/GL/GLProgram.hpp"

class GLRenderModel;
class CameraSetupSettings;
class CameraSetupDataset;


class ViewerGLProgram : public GLProgram
{
public:
	ViewerGLProgram(GLRenderModel* _render_model, CameraSetupSettings* _settings, CameraSetupDataset* _dataset);

	void passUniforms(CameraInfo& camInfo) override;
	void draw() override;

private:
	CameraSetupDataset* dataset = nullptr;
	CameraSetupSettings* settings = nullptr;
};
