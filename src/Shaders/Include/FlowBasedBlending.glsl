#include "Shaders/Include/Utils.glsl"


void getMotionCompensatedTextureCoordinates(in int _useOpticalFlow, in int _flowDownsampled, in vec2 _dim, 
	in sampler2DArray _forwardFlows, in sampler2DArray _backwardFlows, 
	in int _leftNeighbour, in int _rightNeighbour, in vec2 _lTex, in vec2 _rTex, 
	in float _alpha, in int _isEquirect,
	out vec2 _lTexFlow, out vec2 _rTexFlow,
	out vec2 _forwardFlow, out vec2 _backwardFlow, out vec2 _forwardFlowCompensated, out vec2 _backwardFlowCompensated)
{
	vec2 forwardFlow  = vec2(0);
	vec2 backwardFlow = vec2(0);
	
	vec2 compensateForwardFlow  = vec2(0);
	vec2 compensateBackwardFlow = vec2(0);

	vec2 forwardFlowCompensated  = vec2(0);
	vec2 backwardFlowCompensated = vec2(0);

	if (_useOpticalFlow > 0)
	{
		forwardFlow  = fetchFlow(_forwardFlows,  _lTex, _leftNeighbour);
		backwardFlow = fetchFlow(_backwardFlows, _rTex, _rightNeighbour);

		if (_flowDownsampled > 0)
		{
			forwardFlow.x *= 2.0;
			forwardFlow.y *= 2.0;
			backwardFlow.x *= 2.0;
			backwardFlow.y *= 2.0;
		}

		forwardFlow.x /= _dim.x;
		forwardFlow.y /= _dim.y;

		backwardFlow.x /= _dim.x;
		backwardFlow.y /= _dim.y;

		compensateForwardFlow  = _rTex - _lTex;
		compensateBackwardFlow = _lTex - _rTex;

		if(_isEquirect == 1)
		{
			// In 360 images, it can happen that lTex and rTex are across the wraparound,
			// so their difference could be like 0.98 instead of the preferable -0.02.
			// So let's prefer shorter flow vectors along the azimuth (x).
			if(compensateForwardFlow.x < -0.5) compensateForwardFlow.x += 1.;
			if(compensateForwardFlow.x >  0.5) compensateForwardFlow.x -= 1.;
		
			if(compensateBackwardFlow.x < -0.5) compensateBackwardFlow.x += 1.;
			if(compensateBackwardFlow.x >  0.5) compensateBackwardFlow.x -= 1.;
		}

		forwardFlowCompensated  = compensateForwardFlow  - forwardFlow;
		backwardFlowCompensated = compensateBackwardFlow - backwardFlow;
	}

	_lTexFlow = _lTex + (      _alpha) * forwardFlowCompensated;
	_rTexFlow = _rTex + (1.0 - _alpha) * backwardFlowCompensated;

	if(_isEquirect == 1)
	{
		// Handle 360 azimuth wrap-around.
		_lTexFlow.x = mod(_lTexFlow.x, 1);
		_rTexFlow.x = mod(_rTexFlow.x, 1);
	}

	_forwardFlow  = forwardFlow;
	_backwardFlow = backwardFlow;

	_forwardFlowCompensated  = forwardFlowCompensated;
	_backwardFlowCompensated = backwardFlowCompensated;
}


vec4 colourTwoViewSynthesis(
	in int _displayMode,
	in float _alpha,
	in vec3 _worldPos,
	in vec2 _pair,
	in float _phiMinus, in float _phi, in float _phiPlus, in float _range,
	in vec2 _lTex, in vec2 _rTex,
	in float _flowFactor,
	in vec2 _forwardFlow, in vec2 _backwardFlow,
	in vec2 _forwardFlowCompensated, in vec2 _backwardFlowCompensated,
	in vec3 _lColour, in vec3 _rColour
)
{
	switch (displayMode)
	{
		case 0: // showColourMinus = colour of left view
			return vec4(_lColour, 1.);

		case 1: // showColorPlus = colour of right view
			return vec4(_rColour, 1.);

		case 2: // showFlowResult
			return vec4((1. - _alpha) * _lColour + _alpha * _rColour, 1.);
			
		case 3: // showWorldLines = xyz grid lines every 40 cm
			return vec4(0.8 * abs(mod(_worldPos / 10., 4) - 2) / 2., 1.);

		case 4: // showCameraPair
			return vec4(_pair.x, _pair.y, _alpha, 1.);

		case 5: // showLeftTextureCoord
		{
			// "Distance" to principal point
			float w = evaluateGaussian(_lTex.x, 0.2, 0.5);

			//// Colour-code red/green
			//vec3 colour = w * vec3(0., 1., 0.) + (1. - w) * vec3(1., 0., 0.);
			//colour.z = _pair.x;
			//return vec4(colour, 1.);

			return vec4(_lTex, w, 1.);
		}

		case 6: // showRightTextureCoord
		{
			// "Distance" to principal point
			float w = evaluateGaussian(_rTex.x, 0.2, 0.5);

			//// Colour-code red/green
			//vec3 colour = w * vec3(0.0, 1.0, 0.0) + (1.0 - w) * vec3(1.0, 0.0, 0.0);
			//colour.z = _pair.x;
			//return vec4(colour, 1.);

			return vec4(_rTex, w, 1.);
		}

		case 7: // showLRflow
			//return vec4(_forwardFlow, 0., 1.); // raw flow
			return vec4(flowColour(_forwardFlow, _flowFactor), 1.); // colour-coded flow

		case 8: // showRLflow
			//return vec4(_backwardFlow, 0., 1.); // raw flow
			return vec4(flowColour(_backwardFlow, _flowFactor), 1.); // colour-coded flow

		case 9: // showCompensatedLRflow
			//return vec4(_forwardFlowCompensated, 0., 1.); // raw flow
			return vec4(flowColour(_forwardFlowCompensated, _flowFactor), 1.); // colour-coded flow

		case 10: // showCompensatedRLflow
			//return vec4(_backwardFlowCompensated, 0., 1.); // raw flow
			return vec4(flowColour(_backwardFlowCompensated, _flowFactor), 1.); // colour-coded flow

		case 11: // showWorldPositions [metres]
			return vec4(_worldPos / 100., 1.);
	}

//	// TODO: this is missing for now
//	if (_showPhis)
//		_colour = vec3(_phiMinus, _phi, _phiPlus) / 360.0;
//	if (_showPhiRangeAlpha)
//		_colour = vec3(_phi, _range, _alpha);

	// Pinkish sentinel value for undefined display modes.
	return vec4(0.75, 0.25, 0.5, 1.);
}
