#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/GL/GLCameraControl.hpp"
#include "Core/GL/GLProgram.hpp"
#include "Core/GUI/WindowInfo.hpp"
#include "Core/Geometry/Circle.hpp"
#include "Core/Geometry/Plane.hpp"

#include <memory>
#include <vector>


class GLCamera
{
public:
	GLCamera(Eigen::Point3f _centre, Eigen::Matrix3f _rotation, float _fovy, float _aspect, float _near, float _far, Eigen::Matrix3f _basis);
	GLCamera(Eigen::Point3f _centre, Eigen::Matrix3f _rotation, float _fovy, float _aspect, float _near, float _far, Eigen::Matrix3f _basis, Eigen::Vector2i _planeDims, std::shared_ptr<WindowInfo> _windowInfo);
	~GLCamera();


	// called by GLApplication
	void renderPrograms(std::vector<GLProgram*> _gl_programs);


	//We initialise the GLCamera according to the fitted circle of viewpoint centres.
	inline void setCircle(std::shared_ptr<Circle> _circle) { circle = _circle; }

	//if true -> step cameraControl every run().
	bool record = false;
	// Used to describe and perform pre-defined camera paths, i.e. circular-swings
	std::shared_ptr<GLCameraControl> cameraControl;

	//TODO: check
	void update(); //probably updates all the GLPrograms. Needed when reloading shaders on runtime.

	inline std::shared_ptr<Plane> getPlane() const { return plane; }
	float getPlaneDistance();
	void setPlaneDistance(float _distance);
	//in-/decrease the distance of the plane to the camera
	void changePlaneDistance(float _change);
	//float* createVerticesForPlane(std::shared_ptr<Plane> _plane); // shouldn't be necessary anymore --TB

	///////////////////////////
	//Intrinsics interface
	void setFovy(float _fovy);
	float getFovy();
	void setAspect(float _aspect);

	///////////////////////////
	//Camera motion interface
	void moveRight(float amount);
	void moveForward(float amount);
	void moveForwardInPlane(float amount);
	void moveUp(float amount);

	void yaw(float amount);
	void pitch(float amount);
	void roll(float amount);

	void reset();
	void placeToCentre();
	void placeToCentre(Eigen::Vector3f C, Eigen::Matrix3f R);
	///////////////////////////

	void setExtrinsics(Eigen::Vector3f C, Eigen::Matrix3f R);


	inline Eigen::Matrix4f getMVP() const { return MVP; }

	///////////////////////////
	//Extrinsics interface
	inline void setCentre(Eigen::Point3f _centre) { centre = _centre; }
	inline Eigen::Point3f getCentre() const { return centre; }

	inline Eigen::Vector3f getRightDir() const { return right.normalized(); }
	inline Eigen::Vector3f getViewDir() const { return forward.normalized(); }
	inline Eigen::Vector3f getUpDir() const { return up.normalized(); }

	float getPhi();


	void print(bool to_stdout = false);


	//TODO: should become obsolete as soon we use the model matrix M properly
	void updatePlane();


	//TODO: I don't like that.
	//windowInfo should be part of the GLWindow.
	//The GLCamera does not know about the GLWindow though
	std::shared_ptr<WindowInfo> windowInfo;

	//	inline GLRenderModel* getRenderModel() const { return planeRenderModel; }

private:
	//projection parameters
	float fovy;
	float aspect;
	float frustumNear;
	float frustumFar;

	// Look at target from eye, with given up vector.
	Eigen::Matrix4f lookAt(const Eigen::Point3f& eye, const Eigen::Vector3f& target, const Eigen::Vector3f& up);
	// Computes perspective projection matrix.
	Eigen::Matrix4f perspective(float fovy, float aspect, float zNear, float zFar);


	// TODO: Assuming identity model matrix
	//This is really bad and led to bad coding paradigms.
	//!!!
	//For example:
	//Instead of re-creating a "Shape" Plane whenever the camera moves,
	//we could initialise a plane with the GLCamera and apply the camera orientation to it
	//-> 0 cost instead of OpenGL buffer updates.. (D'oh)
	void setModelMatrix();


	void setViewMatrix();
	void setProjectionMatrix();
	void computeMVP();

	std::shared_ptr<Circle> circle; // used for orientations
	std::shared_ptr<Plane> plane;   // billboard. updates when camera moves

	float planeDistance = 1000.f;

	// Translates the camera centre while preserving the camera orientation.
	void move(Eigen::Vector3f dir);

	// Full transformation: MVP = P * inv(V) * M;
	Eigen::Matrix4f MVP; // Model-view-projection matrix
	Eigen::Matrix4f P;   // Projection matrix
	Eigen::Matrix4f V;   // View matrix
	Eigen::Matrix4f M;   // Model matrix

	// lookAt parameters
	Eigen::Vector3f centre;
	Eigen::Vector3f target;

	// Camera coordinate frame
	Eigen::Vector3f up;
	Eigen::Vector3f right;
	Eigen::Vector3f forward;

	// Store initial parameters
	Eigen::Matrix3f initRot;
	Eigen::Point3f initPos;
	float initFovy;
	float initAspect;
	float initNear;
	float initFar;
};
