#version 410 core

// Basic interface for all camera setup operations:
// Contains projection matrices, viewpoints&directions as well as camera images.
#include "Shaders/Include/SharedInterface.glsl"
#include "Shaders/Include/Megastereo.glsl"


void main()
{
	vec2 projMatsDims = vec2(textureSize(projectionMatrices, 0).xy);
	int numberOfProjMats = int(textureSize(projectionMatrices, 0).z);
	int size = int(textureSize(cameraImages, 0).z);
	vec2 dim = vec2(textureSize(cameraImages, 0).xy);

	vec4 X = vec4(worldPos.x, worldPos.y, worldPos.z,1.0);
	
	vec2 X_pol = normalize(vec2(X.x, X.z));

	//Guaranteed to be positive in [0, 360)
	float phi = computePolarAngle(X_pol);
	//float texturePhi = float(texture(cameraActualPhisArray, float(size - 2) / float(size - 1)).x);
	//color = vec4(phi, texturePhi,0,1);
	//return;
	float range = 100.0;
	float phiMinus = -1;
	float phiPlus = -1;
	//x is i, y is j, phi(i) < phi(j)
	float alpha = -1.0;

	float texPhiMin = float(texture(cameraActualPhisArray, 0.0 / float(size - 1)).x);
	float texPhiMax = float(texture(cameraActualPhisArray, float(size - 1) / float(size - 1)).x);
	vec2 pair = searchClosestCameras(cameraActualPhisArray, phi, phiMinus, phiPlus, alpha, size, range);

	//color = vec4(texPhiMin, phi, texPhiMax, 1);
	//color = vec4(phiMinus, phi, phiPlus, 1);
	//color = vec4(alpha, range, phi, 1);
	//return;

	// Clamp
	alpha = clamp(alpha, 0., 1.);

	if ((pair.x == -1) || (pair.y == -1))
	{
		color = vec4(0.0, 1.0, 1.0, 1.0);
		return;
	}

	float leftNeighbour = pair.x;
	float rightNeighbour = pair.y;

	vec3 lColour = vec3(0.0);
	vec3 rColour = vec3(0.0);

	mat4 lP = matrixFetch(projectionMatrices, int(leftNeighbour));
	mat4 rP = matrixFetch(projectionMatrices, int(rightNeighbour));
	
	vec3 cL = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(pair.x) / float(size - 1)));
	vec3 cR = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(pair.y) / float(size - 1)));
	vec3 vL = vec3(0);
	vec3 vR = vec3(0);


	vec2 lTex = vec2(0);
	vec2 rTex = vec2(0);

	if (useEquirectCamera == 0)
	{
		lTex = computeTextureCoord(X, lP, dim);
		rTex = computeTextureCoord(X, rP, dim);
		//color = vec4(lTex.x, lTex.y, 0, 1);
		//return;
	}
	else
	{
		// TODO: We use this in OmniPhotos, so it should work here as well, but it's untested.
		lTex = computeEquirectTextureCoord(X, lP);
		rTex = computeEquirectTextureCoord(X, rP);

//		vL = textureFetch(cameraPositionsAndViewingDirections, vec2(1.0, float(pair.x) / float(size - 1)));
//		//if (dot(normalize(X.xyz - cL), vL) < 0.001)
//		//	return;
//		//aperture around forward direction
//		//if (dot(normalize(X.xyz - cL), vL) > 0.01)
//			lTex = computeEquirectTextureCoord(X.xyz, vL, cL);
//			
//		vR = textureFetch(cameraPositionsAndViewingDirections, vec2(1.0, float(pair.y) / float(size - 1)));
//		//if (dot(normalize(X.xyz - cR), vR) > 0.01)
//			rTex = computeEquirectTextureCoord(X.xyz, vR, cR);
	}

	//color = vec4(lTex.x, lTex.y, 0, 1);
	//return;


	vec2 lTexFlow = vec2(0);
	vec2 rTexFlow = vec2(0);

	vec2 forwardFlow = vec2(0);
	vec2 backwardFlow = vec2(0);
	vec2 forwardFlowCompensated = vec2(0);
	vec2 backwardFlowCompensated = vec2(0);

	getMotionCompensatedTextureCoordinates(useOpticalFlow, flowDownsampled, dim,
		forwardFlows, backwardFlows,
		int(leftNeighbour), int(rightNeighbour), lTex, rTex,
		alpha, useEquirectCamera,
		lTexFlow, rTexFlow,
		forwardFlow, backwardFlow, forwardFlowCompensated, backwardFlowCompensated
	);
	lColour = vec3(colourFetchArray(cameraImages, lTexFlow, int(leftNeighbour)));
	rColour = vec3(colourFetchArray(cameraImages, rTexFlow, int(rightNeighbour)));

	float flowFactor = 0.5;
	color = colourTwoViewSynthesis(displayMode,
		alpha,
		worldPos.xyz,
		pair,
		phiMinus, phi, phiPlus, range,
		lTex, rTex,
		flowFactor,
		forwardFlow, backwardFlow,
		forwardFlowCompensated, backwardFlowCompensated,
		lColour, rColour);
}
