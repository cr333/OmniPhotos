// This is the shared interface for Megastereo and MegaParallax/OmniPhotos rendering.

uniform sampler2DArray projectionMatrices; // Projection from world to image space for each camera.
uniform sampler2DArray cameraImages; // Associated input images.
uniform sampler2D cameraPositionsAndViewingDirections; // Positions and viewing directions for each camera.

uniform sampler2DArray forwardFlows;  // Forward flow fields.
uniform sampler2DArray backwardFlows; // Backward flow fields.

uniform float circleRadius; // Radius of the fitted camera circle [cm].
uniform vec3 circleNormal; // Normal direction of the camera circle.
uniform vec3 desCamPos; // Desired camera position.
uniform vec3 desCamView; // Desired camera viewing direction.

uniform int displayMode;
uniform int useEquirectCamera;
uniform int useOpticalFlow;
uniform int flowDownsampled;
uniform int raysPerPixel; // 0 = Parallax360, 1 = MegaParallax/OmniPhotos
uniform int fadeNearBoundary;

uniform sampler1D cameraActualPhisArray; // The azimuth angles for all cameras.


in vec4 worldPos;


out vec4 color;
