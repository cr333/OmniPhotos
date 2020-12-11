#pragma once

#include <glog/logging.h>

/*!
*  \brief the log class
*/
class Logger
{

public:

	/*!
	* \enum logger::logger_level
	*/
	enum class Logger_level
	{
		Info, Warning, Error, Fatal
	};

	/*!
	 \brief the constructor of logger class.

	 It should be called in main and just once in the whole program (include the *.dll and *.lib).
	 When program crash it will generate output the run-time stack to failure_stack_dump.log.

	 \param logger_name the name of logger object and name the log file after *.log
	 \param level the log larger than the specified threshold level (the first parameter) output, FATAL > ERROR > WARNING > INFO
	*/
	Logger(const std::string& logger_name, const Logger_level level = Logger_level::Info);

	/*!
	* The API to precess the arguments from CLI.
	* If the argv[0] do not assign (NULL), the file name with the timestamp.
	*
	* \param argc the parameters number
	* \param argv the parameters from CLI
	*/
	Logger(int argc, char* argv[]);

	/*!
	* Issues a series of barks.
	*/
	~Logger();

private:
	/*! initial the Glog runtime environment
	*/
	void init_glog_runtime_env(const std::string& log_filename, const Logger_level level);
};

