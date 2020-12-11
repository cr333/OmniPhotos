#version 410 core

// Basic interface for all camera setup operations:
// Contains projection matrices, viewpoints&directions as well as camera images.
#include "Shaders/Include/SharedInterface.glsl"
#include "Shaders/Include/MegaParallax.glsl"


void main()
{
	// vec2 projMatsDims = vec2(textureSize(projectionMatrices, 0).xy);
	// int numberOfProjMats = int(textureSize(projectionMatrices, 0).z);

	// Dimension and number of camera images.
	vec2 dim = vec2(textureSize(cameraImages, 0).xy);
	int size = int(textureSize(cameraImages, 0).z);

	// Point X on the proxy geometry.
	vec4 X = worldPos;

	// Desired camera viewing direction (i.e. forward direction).
	// Used by Parallax360 for flow-based blending computation.
	vec3 dir = desCamView;
	
	// Per-pixel direction to the proxy point X, projected onto the plane of the camera circle.
	// Used by MegaParallax and OmniPhotos for view-dependent flow-based blending computation.
	if (raysPerPixel > 0)
	{
		// Direction from camera to point X.
		dir = X.xyz - desCamPos;

		// Project onto plane of circle to get point X*.
		// CR 2020-03-06: Seems necessary in HMD.
		dir = dir - circleNormal * dot(dir, circleNormal);

		// Normalise to get direction to X*.
		dir = normalize(dir);
	}

	// Find where the direction to X* intersects the camera circle.
	// NB: This is why view synthesis only works correctly inside the camera circle.
	// vec3 inter = intersectCylinder(circleRadius, desCamPos, dir, circleNormal);
	vec3 inter = intersectSphereCentredAtOrigin(circleRadius, desCamPos, dir);
	float phi = computePolarAngle(normalize(vec2(inter.x, inter.z))); // [0, 360)

	// Find the best camera pair in that direction.
	float phiMinus = -1;
	float phiPlus = -1;
	float alpha = -1.0;

	int radius = 5;
	ivec2 pair = searchClosestCameras(cameraActualPhisArray, phi, phiMinus, phiPlus, 
		cameraPositionsAndViewingDirections, desCamPos, dir, circleNormal, radius, size);

	// Left/right camera centres.
	vec3 cL = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(pair.x) / float(size - 1)));
	vec3 cR = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(pair.y) / float(size - 1)));

	// Look up left/right viewing directions (vL/vR), projection matrices (lP/rP) and pinhole texture
	// coordinates (lTex/rTex), and compute the blending weight alpha.
	// NB: Except for the texture coordinates, this logic is independent of the camera model.
	vec3 vL = vec3(0);
	vec3 vR = vec3(0);
	mat4 lP = mat4(0);
	mat4 rP = mat4(0);
	vec2 lTex = vec2(0);
	vec2 rTex = vec2(0);
	fetchPairComputeAlphaAndTexCoords(
		cameraPositionsAndViewingDirections,
		projectionMatrices,
		X,
		dim,
		pair,
		desCamPos, dir,
		circleNormal,
		cL, cR,
		lP, rP,
		lTex, rTex,
		alpha);

	// Compute texture coordinates for equirectangular cameras.
	if (useEquirectCamera == 1)
	{
		lTex = computeEquirectTextureCoord(X, lP);
		rTex = computeEquirectTextureCoord(X, rP);
	}

	vec3 lColour = vec3(0.0);
	vec3 rColour = vec3(0.0);

	vec2 lTexFlow = vec2(0);
	vec2 rTexFlow = vec2(0);
	vec2 forwardFlow = vec2(0);
	vec2 backwardFlow = vec2(0);
	vec2 forwardFlowCompensated = vec2(0);
	vec2 backwardFlowCompensated = vec2(0);
	
	// Apply motion compensation to flow vectors based on proxy geometry.
	getMotionCompensatedTextureCoordinates(useOpticalFlow, flowDownsampled, dim,
		forwardFlows, backwardFlows,
		pair.x, pair.y, lTex, rTex,
		alpha, useEquirectCamera,
		lTexFlow, rTexFlow,
		forwardFlow, backwardFlow, forwardFlowCompensated, backwardFlowCompensated
	);

	// Look up pixel colours at the motion-compensated locations.
	lColour = vec3(colourFetchArray(cameraImages, lTexFlow, pair.x));
	rColour = vec3(colourFetchArray(cameraImages, rTexFlow, pair.y));

	// Blend left and right colours together based on alpha, or show a variety of debug display modes.
	float flowFactor = 0.01;
	color = colourTwoViewSynthesis(displayMode,
		alpha,
		worldPos.xyz,
		pair,
		phiMinus, phi, phiPlus, radius,
		lTex, rTex,
		flowFactor,
		forwardFlow, backwardFlow,
		forwardFlowCompensated, backwardFlowCompensated,
		lColour, rColour);

	// Fade out when approaching the camera circle.
	if(fadeNearBoundary == 1)
		color.xyz *= 0.4 + 0.6 * smoothstep(circleRadius, circleRadius - 15, length(desCamPos));
}
