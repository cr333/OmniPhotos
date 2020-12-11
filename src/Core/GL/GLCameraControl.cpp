#include "GLCameraControl.hpp"

#include "3rdParty/fs_std.hpp"
#include "Core/GL/GLCamera.hpp"
#include "Utils/Logger.hpp"
#include "Utils/STLutils.hpp"
#include "Utils/Utils.hpp"

#include <fstream>
#include <iomanip>


PathCommand::PathCommand(const std::string& filename, const std::string& dataset_name)
{
	//// Toy example for testing.
	//for (float i = 0; i < 100; i += 1.f)
	//{
	//	//Eigen::Matrix3f R = Eigen::Matrix3f::Identity();
	//	//Eigen::Vector3f C(0, 0, i - 50);
	//	Eigen::Matrix3f R = Eigen::AngleAxisf(float(2. * i * M_PI / 100.), Eigen::Vector3f::UnitY()).matrix();
	//	Eigen::Vector3f C(0, 0, 0);

	//	std::stringstream name;
	//	name << std::setfill('0') << std::setw(4) << i;

	//	poses.push_back({ name.str(), R, C });
	//}

	readPathCSV(filename, dataset_name);
}


bool PathCommand::step(GLCamera* _glCam, float _deltaTime)
{
	// Finished path? => done
	if (frame < 0 || frame >= poses.size())
	{
		screenshot_name = "";
		return true;
	}

	const auto& pose = poses[frame];
	//_glCam->moveToAndOrient(pose.C, pose.R);
	_glCam->setExtrinsics(pose.C, pose.R);

	screenshot_name = path_name + "-" + pose.name + ".png";
	frame++;

	// Finished path?
	return (frame >= poses.size());
}


bool PathCommand::readPathCSV(const std::string& filename, const std::string& dataset_name)
{
	// Check if the file can be read.
	std::ifstream ifs(filename);
	if (!ifs.is_open())
	{
		LOG(WARNING) << "Could not open '" << filename << "'";
		return false;
	}

	// Read and parse the file line by line.
	std::string line;
	while (getline(ifs, line))
	{
		std::vector<std::string> tokens = split(line, ',');

		if (tokens.size() != 1 + 9 + 3)
		{
			LOG(WARNING)
			    << "Unexpected number of tokens -- expected 13, got " << tokens.size() << ":"
			    << "\n\nFile:\n"
			    << filename
			    << "\n\nLine:\n"
			    << line << "\n\n";
			return false;
		}

		Eigen::Matrix3f R;
		R << stof(tokens[1]), stof(tokens[2]), stof(tokens[3]),
		     stof(tokens[4]), stof(tokens[5]), stof(tokens[6]),
		     stof(tokens[7]), stof(tokens[8]), stof(tokens[9]);

		Eigen::Vector3f C;
		C << stof(tokens[10]), stof(tokens[11]), stof(tokens[12]);

		poses.push_back({ tokens[0], R, C });
	}
	ifs.close();

	// Extract path name from dataset name and CSV filename (minus extension).
	path_name = fs::path(filename).stem().generic_string();
	if (!dataset_name.empty())
		path_name = dataset_name + "-" + path_name;

	return true;
}


CircleSwingCommand::CircleSwingCommand(float angularSpeed)
{
	speed = new float;
	*speed = angularSpeed;
}


CircleSwingCommand::~CircleSwingCommand()
{
	delete speed;
}


bool CircleSwingCommand::step(GLCamera* gl_cam, float delta_time)
{
	if (record_frames)
	{
		// Export frames at regularly intervals, independent of rendering speed.
		std::stringstream ss;
		ss << dataset_name << (dataset_name.empty() ? "" : "-") << "swing";
		export_path_name = ss.str() + ".csv";
		ss << "-" << std::setfill('0') << std::setw(4) << frame << ".png";
		screenshot_name = ss.str();
		lastPhi = (*speed) * (frame / 60.f); // emulating 60 fps
		frame++;
	}
	else
	{
		export_path_name = "";
		screenshot_name = ""; // don't record frames
		lastPhi += (*speed) * delta_time;
	}

	// Move camera on a circle.
	Eigen::Vector3f C = radius * Eigen::Vector3f(sinf(lastPhi), 0.f, -cosf(lastPhi));

	// Quick construction of look-at matrix.
	Eigen::Vector3f forward = -basis * (lookAt - C).normalized();
	//// Hacked path for teaser animation on GenoaCathedral
	//// (circle radius: 32cm, Look-at dir: -135, look-at rad: 1000cm)
	////Eigen::Vector3f forward = -base * (lookAt + Eigen::Vector3f(2 * 58 * cosf(lastPhi), -200 * sinf(lastPhi) - 300, 2 * 81 * cosf(lastPhi)) - C).normalized();
	if (forward.norm() < 0.5) forward = -Eigen::Vector3f::UnitZ();
	const Eigen::Vector3f up = basis.col(1);
	const Eigen::Vector3f right = forward.cross(up);

	// Fill rows of the rotation matrix.
	Eigen::Matrix3f R;
	R << right.transpose(), up.transpose(), forward.transpose();

	gl_cam->setExtrinsics(C, R);

	return false; // not done (infinite animation)
}


TranslateSwingCommand::TranslateSwingCommand(float angularSpeed)
{
	speed = new float;
	*speed = angularSpeed;
}


TranslateSwingCommand::~TranslateSwingCommand()
{
	delete speed;
}


bool TranslateSwingCommand::step(GLCamera* _glCam, float _deltaTime)
{
	if (record_frames)
	{
		// Export frames at regularly intervals, independent of rendering speed.
		std::stringstream ss;
		ss << dataset_name << (dataset_name.empty() ? "" : "-") << "translate";
		export_path_name = ss.str() + ".csv";
		ss << "-" << std::setfill('0') << std::setw(4) << frame << ".png";
		screenshot_name = ss.str();
		lastPhi = (*speed) * (frame / 60.f); // emulating 60 fps
		frame++;
	}
	else
	{
		export_path_name = "";
		screenshot_name = ""; // don't record frames
		lastPhi += (*speed) * _deltaTime;
	}

	// Move camera on an angled line.
	Eigen::Vector3f C = radius * sinf(lastPhi) * Eigen::Vector3f(sinf(angle), 0.f, -cosf(angle));

	// Quick construction of look-at matrix.
	Eigen::Vector3f forward = -basis * (lookAt - C).normalized();
	if (forward.norm() < 0.5) forward = -Eigen::Vector3f::UnitZ();
	const Eigen::Vector3f up = basis.col(1);
	const Eigen::Vector3f right = forward.cross(up);

	// Fill rows of the rotation matrix.
	Eigen::Matrix3f R;
	R << right.transpose(), up.transpose(), forward.transpose();

	_glCam->setExtrinsics(C, R);

	return false; // not done (infinite animation)
}


GLCameraControl::GLCameraControl(GLCamera* _gl_cam, std::shared_ptr<InputHandler> _handler)
{
	gl_cam = _gl_cam;
	handler = _handler;
}


GLCameraControl::~GLCameraControl()
{
}


void GLCameraControl::addCommand(std::shared_ptr<Command> _command)
{
	commands.push_back(_command);
}


void GLCameraControl::init(std::vector<std::shared_ptr<Command>> _commands)
{
	recording_session = _commands;
	commands.clear();

	for (int i = 0; i < _commands.size(); i++)
	{
		addCommand(_commands[i]);
	}
}


void GLCameraControl::step()
{
	// If we don't have any commands left in the queue, repeat the sequence of commands again.
	if (commands.empty())
	{
		// Unless we're following a loaded path, in which case we'll stop when we're done.
		if (recording_session.size() > 0 && std::dynamic_pointer_cast<PathCommand>(recording_session[0]) != nullptr)
		{
			gl_cam->record = false;
			return;
		}

		// Fill up the queue of commands again.
		for (int i = 0; i < recording_session.size(); i++)
			addCommand(recording_session[i]);

		LOG_IF(WARNING, commands.empty()) << "Command queue is empty";
	}

	// Execute the first command in the queue.
	std::shared_ptr<Command> cmd = commands[0];
	bool finished = cmd->step(gl_cam, (float)handler->deltaTime);
	screenshot_name = cmd->screenshot_name;

	// Export camera pose to path CSV.
	if (cmd->export_path_name.empty() == false)
	{
		// Construct filename for path CSV export.
		fs::path projectSrcDir = fs::path(determinePathToSource());
		fs::path screenshotDir = projectSrcDir / ".." / "Screenshots";
		std::string pathFilename = (screenshotDir / cmd->export_path_name).generic_string();
		if (!fs::exists(screenshotDir))
			fs::create_directories(screenshotDir);

		// Add one line for the current camera pose.
		std::ofstream pathFile(pathFilename.c_str(), std::ios_base::app);
		Eigen::IOFormat csvFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ",", ",");
		pathFile << cmd->screenshot_name.substr(0, cmd->screenshot_name.length() - 4) // remove ".png" from name
		         << "," << gl_cam->getRightDir().format(csvFormat)
		         << "," << gl_cam->getUpDir().format(csvFormat)
		         << "," << gl_cam->getViewDir().format(csvFormat)
		         << "," << gl_cam->getCentre().format(csvFormat) << "\n";
	}

	// Remove finished command from queue.
	if (finished)
		commands.erase(commands.begin());
}
