#include "GLCamera.hpp"

#include "Core/GL/GLProgram.hpp"
#include "Core/Geometry/Primitive.hpp"
#include "Utils/ErrorChecking.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

#include <iomanip>


GLCamera::GLCamera(Eigen::Point3f _centre, Eigen::Matrix3f _rotation, float _fovy, float _aspect, float _near, float _far, Eigen::Matrix3f _basis)
{
	initRot = _rotation;
	initPos = _centre;
	initFovy = _fovy;
	initAspect = _aspect;
	initNear = _near;
	initFar = _far;
	centre = _centre;

	//this is dangerous. right is not used explicitly when creating the view matrix.
	//It's all about forward and up and whether the camera is left or right oriented
	right = _rotation.col(0);
	up = _rotation.col(1);
	forward = _rotation.col(2);

	fovy = degToRad(_fovy);
	aspect = _aspect;
	frustumNear = _near;
	frustumFar = _far;

	setProjectionMatrix();
	target = centre + forward;

	circle = std::make_shared<Circle>(Eigen::Point3f(0.0f, 0.0f, 0.0f), _basis.col(1), _basis.col(2), 10.0f);

	setViewMatrix();
	setModelMatrix();
	computeMVP();
}


GLCamera::GLCamera(Eigen::Point3f _centre, Eigen::Matrix3f _rotation,
                   float _fovy, float _aspect, float _near, float _far,
                   Eigen::Matrix3f _basis, Eigen::Vector2i _planeDims,
                   std::shared_ptr<WindowInfo> _windowInfo) :
    GLCamera(_centre, _rotation, _fovy, _aspect, _near, _far, _basis)
{
	float focal = (float)_planeDims.y() / (2.0f * tan(fovy / 2.0f));
	planeDistance = focal;
	windowInfo = _windowInfo;
	plane = std::make_shared<Plane>(getCentre() + getViewDir() * planeDistance, -getViewDir(), 10 * _planeDims);
	plane->createRenderModel("GLCamera plane");
	updatePlane();
	reset(); // TODO: bad naming
}


GLCamera::~GLCamera()
{
}


void GLCamera::renderPrograms(std::vector<GLProgram*> _gl_programs)
{
	for (GLProgram* prog : _gl_programs)
	{
		if (prog->display)
		{
			GLRenderModel* model = prog->getActiveRenderModel();
			if (model)
			{
				ErrorChecking::checkGLError();
				CameraInfo camInfo = CameraInfo(getMVP(), getCentre(), getViewDir());
				prog->passUniforms(camInfo);
				ErrorChecking::checkGLError();
				prog->draw();
				ErrorChecking::checkGLError();
			}
		}
	}
}


float GLCamera::getPhi()
{
	return cartesianToAngle(Eigen::Vector2f(getViewDir().x(), getViewDir().z()));
}


void GLCamera::placeToCentre()
{
	centre = Eigen::Vector3f(0, 0, 0);
	up = Eigen::Vector3f(0, 1, 0);
	forward = Eigen::Vector3f(0, 0, -1);
	target = centre + forward;
	update();
}


void GLCamera::placeToCentre(Eigen::Vector3f C, Eigen::Matrix3f R)
{
	centre = C;
	up = R.col(1);
	forward = R.col(2);
	target = centre + forward;
	update();
}


void GLCamera::reset()
{
	right   = initRot.col(0);
	up      = initRot.col(1);
	forward = initRot.col(2);

	centre = initPos;
	target = centre + forward;

	fovy = degToRad(initFovy);
	aspect = initAspect;
	frustumNear = initNear;
	frustumFar = initFar;

	update();
}


void GLCamera::setExtrinsics(Eigen::Vector3f C, Eigen::Matrix3f R)
{
	centre = C;
	//this looks reasonable assuming that OpenGL matrices are row-major.
	//We do our math usually in column space -> that's where the transpose comes from
	//It implies that we use for the normal stuff row major as well, which does not make much sense with the Math then, no?
	right = R.row(0);
	up = R.row(1);
	forward = R.row(2);
	target = centre + forward;
	update();
}


void GLCamera::setModelMatrix()
{
	M = Eigen::Matrix4f::Identity();
}


void GLCamera::setProjectionMatrix()
{
	P = perspective(fovy, aspect, frustumNear, frustumFar);
}


void GLCamera::setViewMatrix()
{
	V = lookAt(centre, target, up);

	forward = (target - centre).normalized();
	right = forward.cross(up).normalized();
	up = right.cross(forward).normalized();
	right = forward.cross(up).normalized();
}


void GLCamera::computeMVP()
{
	MVP = P * V * M;
}


// This function is modelled after glm::lookAtRH(eye, center, up).
Eigen::Matrix4f GLCamera::lookAt(const Eigen::Point3f& eye, const Eigen::Vector3f& target, const Eigen::Vector3f& up)
{
	Eigen::Vector3f f = (target - eye).normalized(); // forward
	Eigen::Vector3f s = f.cross(up).normalized();    // right (side)
	Eigen::Vector3f u = s.cross(f);                  // up

	Eigen::Matrix4f matrix = Eigen::Matrix4f::Identity();
	matrix(0, 0) = s.x();
	matrix(0, 1) = s.y();
	matrix(0, 2) = s.z();
	matrix(1, 0) = u.x();
	matrix(1, 1) = u.y();
	matrix(1, 2) = u.z();
	matrix(2, 0) = -f.x();
	matrix(2, 1) = -f.y();
	matrix(2, 2) = -f.z();
	matrix(0, 3) = -s.dot(eye); // t = -RC = -[s'; u'; -f'] * eye
	matrix(1, 3) = -u.dot(eye);
	matrix(2, 3) = f.dot(eye);
	return matrix;
}


// This function is modelled after glm::perspectiveRH(T fovy, T aspect, T zNear, T zFar),
// assuming GLM_DEPTH_CLIP_SPACE := GLM_DEPTH_NEGATIVE_ONE_TO_ONE (the default value).
Eigen::Matrix4f GLCamera::perspective(float fovy, float aspect, float zNear, float zFar)
{
	assert(abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

	float tanHalfFovy = tan(fovy / 2.0f);

	Eigen::Matrix4f matrix = Eigen::Matrix4f::Zero();
	matrix(0, 0) = 1.0f / (aspect * tanHalfFovy);
	matrix(1, 1) = 1.0f / tanHalfFovy;
	matrix(2, 2) = -(zFar + zNear) / (zFar - zNear);
	matrix(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix(3, 2) = -1.0f;
	return matrix;
}


void GLCamera::setFovy(float _fovy)
{
	LOG(INFO) << std::fixed << std::setprecision(3)
	          << "Setting fovy = " << _fovy << " rad = " << (_fovy / M_PI * 180) << " deg";

	fovy = _fovy;

	// need to update projection matrix
	setProjectionMatrix();
	computeMVP();
	print();
}


// Returns the vertical field of view in radians.
float GLCamera::getFovy()
{
	return fovy;
}


void GLCamera::update()
{
	ErrorChecking::checkGLError();
	setViewMatrix();
	setProjectionMatrix();
	computeMVP();
	updatePlane();
	print();
}


void GLCamera::updatePlane()
{
	plane->setCentre(getCentre() + planeDistance * getViewDir());
	plane->setNormal(-getViewDir());

	//A bit hacky but does the job.
	//TODO: this could become a simple update of the Planes's Model matrix.
	//The Model matrix will go into the Primitive I'd say.
	std::vector<float> planeVertices = plane->generateVertices(getUpDir(), getRightDir());
	plane->vertices = planeVertices;
	plane->fillVertex3DVectorFromVertices();
	plane->createRenderModel("GLCamera plane");
	ErrorChecking::checkGLError();
}


void GLCamera::changePlaneDistance(float _change)
{
	float distance = (plane->getCentre() - getCentre()).norm();
	planeDistance = distance + _change;
	updatePlane();
}


void GLCamera::setPlaneDistance(float _distance)
{
	planeDistance = _distance;
	updatePlane();
}


float GLCamera::getPlaneDistance()
{
	return planeDistance;
}


void GLCamera::move(Eigen::Vector3f dir)
{
	centre = centre + dir;
	target = target + dir;

	update();
}


void GLCamera::moveRight(float amount)
{
	move(amount * right);
}


void GLCamera::moveUp(float amount)
{
	move(amount * up);
}


void GLCamera::moveForward(float amount)
{
	move(amount * forward);
}


void GLCamera::moveForwardInPlane(float amount)
{
	const auto dir = circle->getNormal().cross(right);
	move(amount * dir);
}


/// Rotates the camera about the world up-vector by 'amount' degrees.
void GLCamera::yaw(float amount)
{
	up = initRot.col(1); // world up
	auto rotation = Eigen::AngleAxisf(degToRad(amount), up);

	// Apply the rotation at the camera centre.
	target = rotation * (target - centre) + centre;

	update();
}


/// Rotates the camera about its right-vector by 'amount' degrees.
void GLCamera::pitch(float amount)
{
	auto rotation = Eigen::AngleAxisf(degToRad(amount), right);

	// Apply the rotation at the camera centre.
	target = rotation * (target - centre) + centre;

	update();
}


/// Rotates the camera about its forward-vector by 'amount' degrees.
void GLCamera::roll(float amount)
{
	auto rotation = Eigen::AngleAxisf(degToRad(amount), forward);

	// Rotate camera about forward vector.
	up = rotation * up;

	update();
}


void GLCamera::print(bool to_stdout)
{
	Eigen::IOFormat CleanFmt(Eigen::StreamPrecision, 0, ", ", "\n", "[", "]");

	std::stringstream ss;
	ss
	    << std::fixed << std::setprecision(3)
	    << "Rendering camera:\n"
	    << "\n"
	    //<< "Model matrix, 4x4 homogeneous:\n" // always identity matrix
	    //<< M.format(CleanFmt)
	    //<< "\n\n"
	    << "View matrix, 4x4 homogeneous:\n"
	    << V.format(CleanFmt)
	    << "\n\n"
	    << "Projection matrix, 4x4 homogeneous:\n"
	    << P.format(CleanFmt)
	    << "\n"
	    << "\nCamera centre     : " << getCentre().transpose().format(CleanFmt)
	    << "\nForward direction : " << forward.transpose().format(CleanFmt)
	    << "\nUp direction      : " << up.transpose().format(CleanFmt)
	    << "\nRight direction   : " << right.transpose().format(CleanFmt)
	    << "\n"
	    << "\nCamera fovy [deg] : " << radToDeg(fovy)
	    << "\nPlane dist. [cm]  : " << planeDistance
	    << "\nCamera phi [deg]  : " << getPhi()
	    << "\n"
	    << std::endl;

	if (to_stdout)
		LOG(INFO) << ss.str();
	else
		VLOG(1) << ss.str();
}


void GLCamera::setAspect(float _aspect)
{
	aspect = _aspect;
	setProjectionMatrix();
	computeMVP();
}
