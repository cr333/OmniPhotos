#pragma once

#include "Core/GUI/InputHandler.hpp"
#include "Core/LinearAlgebra.hpp"

#include <memory>
#include <vector>


class Command;
class GLCamera; // break circular dependency


class GLCameraControl
{
public:
	GLCameraControl(GLCamera* _gl_cam, std::shared_ptr<InputHandler> _handler);
	~GLCameraControl();

	void addCommand(std::shared_ptr<Command> _command);
	void init(std::vector<std::shared_ptr<Command>> _commands);

	//Take first Command from the vector.
	//step that command
	//if command finishes (returns true)
	//remove command from vector
	void step();

	// If this is set to a non-empty string, then a screenshot with that name will be saved after rendering the frame.
	std::string screenshot_name = "";

	// Queue of commands to be executed in order.
	std::vector<std::shared_ptr<Command>> recording_session;

private:
	std::vector<std::shared_ptr<Command>> commands;
	GLCamera* gl_cam = nullptr;
	std::shared_ptr<InputHandler> handler;
};


class Command
{
public:
	//translation and rotation could happen at different speeds.
	//and should be changed on runtime -> passing references to UI translation and rotation speed.
	float* speed = nullptr;

	//this is the key.
	//The GLCamera holds commands and triggers step until it's done.
	virtual bool step(GLCamera* _glCam, float _deltaTime) = 0;

	// If this is set to a non-empty string, then a path CSV with that name will be exported during the animation.
	std::string export_path_name = "";

	// If this is set to a non-empty string, then a screenshot with that name will be saved after rendering the frame.
	std::string screenshot_name = "";
};


struct NamedPose
{
	std::string name;
	Eigen::Matrix3f R;
	Eigen::Vector3f C;
};


class PathCommand : public Command
{
public:
	PathCommand(const std::string& filename, const std::string& dataset_name = "");

	bool step(GLCamera* gl_cam, float delta_time) override final;

	bool readPathCSV(const std::string& filename, const std::string& dataset_name = "");


private:
	std::string path_name = "UnnamedPath";
	std::vector<NamedPose> poses;
	int frame = 0;
};


class CircleSwingCommand : public Command
{
public:
	CircleSwingCommand(float angular_speed = M_PI);
	~CircleSwingCommand();

	bool step(GLCamera* gl_cam, float delta_time) override final;

	float radius = 10.f; // [cm]
	float lastPhi = 0.f; // [rad]
	Eigen::Vector3f lookAt = -1000 * Eigen::Vector3f::UnitZ();
	Eigen::Matrix3f basis; // orientation of fitted circle

	bool record_frames = false;
	std::string dataset_name;
	float frame = 0;
};


// TODO: Refactor so it doesn't refer to angles.
class TranslateSwingCommand : public Command
{
public:
	TranslateSwingCommand(float angular_speed = M_PI);
	~TranslateSwingCommand();

	bool step(GLCamera* gl_cam, float delta_time) override final;

	float radius = 10.f; // [cm]
	float angle = 0.f;   // [rad]
	float lastPhi = 0.f; // [rad]
	Eigen::Vector3f lookAt = -1000 * Eigen::Vector3f::UnitZ();
	Eigen::Matrix3f basis; // orientation of fitted circle

	bool record_frames = false;
	std::string dataset_name;
	float frame = 0;
};
