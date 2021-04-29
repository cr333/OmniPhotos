#pragma once

#include "3rdParty/Eigen.hpp"


/** 
 * @brief Used to model 3D positions, points or vertices.
 * @param pos 3D Euclidean coordinate
 * @param colour RGB colour value
 * @param error Stores reconstruction error when reading in sparse point clouds via COLMAP. Undefined for OpenVSLAM.
 */
struct Point3D
{
	Point3D();
	Point3D(int _id, const Eigen::Vector3f& _pos, const Eigen::Vector3f& _colour, float _error);
	Point3D(float x, float y, float z, float r = 0.5f, float g = 0.5f, float b = 0.5f);
	Point3D(Eigen::Vector3f p, Eigen::Vector3f c = Eigen::Vector3f { 0, 0, 0 });

	// Basically vertices of texture geometry
	Point3D(Eigen::Vector3f _pos, Eigen::Vector3f _normal, Eigen::Vector2f _tex);

	// Euclidean point in 3D
	Eigen::Vector3f pos;

	// Per-vertex colour
	Eigen::Vector3f colour;

	/**
	 * Per-vertex normal.
	 */
	Eigen::Vector3f normal;

	/**
	 * Texture coordinates.
	 */
	Eigen::Vector2f tex;

	// Point ID
	int id = -1;

	// Reprojection error on tracked images
	float error = -1;
};
