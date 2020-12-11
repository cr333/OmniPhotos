#pragma once


enum class FlowMethod
{
	BroxCUDA  = 0,
	DIS       = 1,
	Farneback = 2
};


// GPU Brox parameters
struct BroxFlowParameters
{
	float alpha = 0.5f;
	float gamma = 50.f;
	float scaleFactor = 0.85f;
	int innerIterations = 10;
	int outerIterations = 150;
	int solverIterations = 5;
};
