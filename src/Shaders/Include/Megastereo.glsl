#include "Shaders/Include/FlowBasedBlending.glsl"


//I want the two enclosing cameras for a given phi including their range
vec2 searchClosestCameras(in sampler1D _cameraActualPhisArray, 
	in float phi, inout float phiLeft, inout float phiRight, 
	inout float alpha, in int size, inout float range)
{
	int lowerIndex = 0;
	int upperIndex = size - 1;

	//cameras are sorted in a ascending phi which goes clockwise

	//assume that phis are between [0, 360)
	float minPhi = float(texture(_cameraActualPhisArray, 0.0 / float(size - 1)).x);
	float maxPhi = float(texture(_cameraActualPhisArray, float(size - 1) / float(size - 1)).x);

	// wrap around
	if (phi < minPhi)
	{
		range = 360.0 - (maxPhi - minPhi);
		alpha = ((phi + 360.0) - maxPhi) / range;
		phiLeft = float(texture(_cameraActualPhisArray, float(size-1) / float(size - 1)).x);
		phiRight = float(texture(_cameraActualPhisArray, float(0) / float(size - 1)).x);
		return vec2(size - 1, 0);
	}
	
	// wrap around
	if (phi > maxPhi)
	{
		range = 360.0 - (maxPhi - minPhi);
		alpha = (phi - maxPhi) / range;
		phiLeft = float(texture(_cameraActualPhisArray, float(size - 1) / float(size - 1)).x);
		phiRight = float(texture(_cameraActualPhisArray, float(0) / float(size - 1)).x);
		return vec2(size - 1, 0);
	}

	//int radius = size;
	int index;
	float phiTmp;
	bool lowerIndexFound = false;
	int j = 0;
	float phiDiff;
	//given a target "phi" we search the actualPhisArray to find the "closest" match.
	while ((!lowerIndexFound) && (j < size))
	{
		//current phi
		phiTmp = float(texture(_cameraActualPhisArray, float(j) / float(size - 1)).x);
		//difference
		phiDiff = (phiTmp - phi);
		// got phiTmp > phi
		if (phiDiff > 0)
		{
			//take the phi from before as best match
			lowerIndex = j - 1;
			lowerIndexFound = true;
		}
		j++;
	}
	upperIndex = (lowerIndex + 1) % size;
	phiLeft = float(texture(_cameraActualPhisArray, float(lowerIndex) / float(size - 1)).x);
	phiRight = float(texture(_cameraActualPhisArray, float(upperIndex) / float(size - 1)).x);
	range = phiRight - phiLeft;
	alpha = (phi - phiLeft) / range;
	if (range < 0.0)
	{
		range = (phiRight - phiLeft) + 360.0;
		alpha = (phi - phiLeft) / range;
		if (range < 0.0)
			alpha = 1.0 - alpha;
	}
	if (range == 0)
	{
		return vec2(-1);
	}
	return vec2(lowerIndex, upperIndex);
}
