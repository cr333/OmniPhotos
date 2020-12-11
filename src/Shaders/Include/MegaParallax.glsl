#include "Shaders/Include/FlowBasedBlending.glsl"


float computePhiInAFancyWay(in vec3 C, in vec3 v, in vec3 n)
{
	vec3 C_star = C - n * dot(C, n);
	vec3 x_star = v - n * dot(v, n);
	float leftArg = dot(cross(x_star, C_star), n);
	float rightArg = dot(x_star, C_star);
	float phi = atan(leftArg, rightArg);
	return phi;
}


ivec2 searchClosestCameras(in sampler1D _cameraActualPhisArray,  
	in float phi, inout float phiMinus, inout float phiPlus, 
	in sampler2D _cameraPositionsAndViewingDirections, in vec3 desPos, in vec3 desView, 
	in vec3 circleNormal, in int radius, in int size)
{
	int lowerIndex = size - 1;

	for(int j = 0; j < size; j++)
	{
		float phiTmp = float(texture(_cameraActualPhisArray, float(j) / float(size - 1)).x);
		if (phiTmp > phi)
		{
			lowerIndex = j - 1;
			break;
		}
	}

	int upperIndex;
	for(int i = -radius; i <= radius; i++)
	{ 
		int left = (lowerIndex + i + size) % size;
		int right = (left + 1) % size; 

		// Verify the determined camera pair by the signed angle
		vec3 cL = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(left) / float(size - 1)));
		vec3 dL = normalize(cL - desPos);

		float phi_L = computePhiInAFancyWay(dL, desView, circleNormal);
		if (abs(phi_L) < (3.141 / 2.0))
		{
			vec3 cR = textureFetch(cameraPositionsAndViewingDirections, vec2(0.0, float(right) / float(size - 1)));
			vec3 dR = normalize(cR - desPos);
			float phi_R = computePhiInAFancyWay(dR, desView, circleNormal);
			if (phi_L * phi_R <= 0.f) // tests for different signs
			{
				lowerIndex = left;
				upperIndex = right;
				break;
			}
		}
	}

	phiMinus = float(texture(_cameraActualPhisArray, float(lowerIndex) / float(size - 1)).x);
	phiPlus  = float(texture(_cameraActualPhisArray, float(upperIndex) / float(size - 1)).x);

	return ivec2(lowerIndex, upperIndex);
}


// Computed the blending weight alpha for a given desired camera (pos, view) w.r.t. to a camera pair
// (defined by camera centre coordinates).
float computeTranslationAlphaByDirectionSimilarity(in vec3 camPosMinus, in vec3 camPosPlus, in vec3 desPos, in vec3 desView, in vec3 circleNormal)
{
	// Project direction vectors onto the plane of the camera circle.
	vec3 d = normalize(desView);
	vec3 dProj = circleNormal * dot(d, circleNormal);
	d = normalize(d - dProj);

	vec3 wL = normalize(camPosMinus - desPos);
	vec3 wLProj = circleNormal * dot(wL, circleNormal);
	wL = normalize(wL - wLProj);

	vec3 wR = normalize(camPosPlus - desPos);
	vec3 wRProj = circleNormal * dot(wR, circleNormal);
	wR = normalize(wR - wRProj);

	float left  = acos(dot(wL, d));  // angle between directions to left camera + desired ray direction
	float range = acos(dot(wL, wR)); // angle between direction to the left and right cameras
	float alpha = left / range; // ratio of angles
	return alpha;
}


void fetchPairComputeAlphaAndTexCoords(
	in sampler2D _cameraPositionsAndViewingDirections,
	in sampler2DArray _projectionMatrices,
	in vec4 _X,
	in vec2 _dim,
	in vec2 _pair,
	in vec3 _cD, in vec3 _vD,
	in vec3 _circleNormal,
	inout vec3 _cL, inout vec3 _cR,
	inout mat4 _PL, inout mat4 _PR,
	inout vec2 _lTex, inout vec2 _rTex,
	inout float _alpha)
{
	int size = int(textureSize(_cameraPositionsAndViewingDirections, 0).y);

	_cL = textureFetch(_cameraPositionsAndViewingDirections, vec2(0.0, float(_pair.x) / float(size - 1)));
	_cR = textureFetch(_cameraPositionsAndViewingDirections, vec2(0.0, float(_pair.y) / float(size - 1)));

	_alpha = computeTranslationAlphaByDirectionSimilarity(_cL, _cR, _cD, _vD, _circleNormal);
	_alpha = clamp(_alpha, 0., 1.);

	_PL = matrixFetch(_projectionMatrices, int(_pair.x));
	_lTex = computeTextureCoord(_X, _PL, _dim);
	_PR = matrixFetch(_projectionMatrices, int(_pair.y));
	_rTex = computeTextureCoord(_X, _PR, _dim);
}
