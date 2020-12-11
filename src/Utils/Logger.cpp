#include "Logger.hpp"

#include "3rdParty/fs_std.hpp"
#include "Utils/Exceptions.hpp"

#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>


Logger::Logger(int argc, char* argv[])
{
	std::string log_filename;
	if (argc > 0 && argv != nullptr)
	{
		fs::path exe_name(argv[0]);
		std::string filename = exe_name.filename().string(); // get the file name
		log_filename.assign(filename);
	}
	else
	{
		// generate the timestamp as the log filename
		std::time_t t = std::time(nullptr);
		char filename_char[100];
		std::strftime(filename_char, sizeof(filename_char), "%Y%m%d%H%M%S", std::localtime(&t));
		log_filename.assign(filename_char);
		std::cout << "WARNING: log file do not set. name with timestamp: " << filename_char << std::endl;
	}
	init_glog_runtime_env(log_filename, Logger_level::Info);
}

Logger::Logger(const std::string& logger_name, const Logger_level level)
{
	// Datetime suffix for unique log files.
	char datetime[100];
	std::time_t t = std::time(nullptr);
	std::strftime(datetime, sizeof(datetime), "-%Y%m%d-%H%M%S", std::localtime(&t));

	init_glog_runtime_env(logger_name + datetime, level);
}

void Logger::init_glog_runtime_env(const std::string& logger_name, const Logger_level level)
{
	google::InitGoogleLogging(logger_name.c_str());

	// Disable timestamp, as we use our own.
	FLAGS_timestamp_in_logfile_name = false;

	// set the threshold of GLOG
	if (level == Logger_level::Info)
		FLAGS_stderrthreshold = google::GLOG_INFO;
	else if (level == Logger_level::Warning)
		FLAGS_stderrthreshold = google::GLOG_WARNING;
	else if (level == Logger_level::Error)
		FLAGS_stderrthreshold = google::GLOG_ERROR;
	else if (level == Logger_level::Fatal)
		FLAGS_stderrthreshold = google::GLOG_FATAL;
	else
		RUNTIME_EXCEPTION("logger level error!");

	// set color messages logged to stderr (if supported by terminal)
	FLAGS_colorlogtostderr = true;

	// set whether log to both a log file and to stderr
	FLAGS_alsologtostderr = false;

	// set whether just log to stderr and do not output to log file
	//FLAGS_logtostderr = true;

	std::string log_file_name;
	if (logger_name.empty())
		log_file_name = "log";
	else
		log_file_name = logger_name;

	// change the destination file of google::GLOG_INFO, set output log to file prefix
	// the log level larger than the specified threshold level (the first parameter) output  to the log file
	// google::GLOG_ERROR > google::GLOG_WARNING > google::GLOG_INFO
	google::SetLogDestination(FLAGS_stderrthreshold, log_file_name.c_str());

	// sets the maximum number of seconds which logs may be buffered for.
	FLAGS_logbufsecs = 0;

	// sets the maximum log file size (in 100MB).
	FLAGS_max_log_size = 100;

	// sets to avoid logging to the disk if the disk is full.
	FLAGS_stop_logging_if_full_disk = true;

	// extension as a suffix to log file name
	google::SetLogFilenameExtension(".log");

	// catch core dumped
	google::InstallFailureSignalHandler();

	// output SIGSEGV to file
	google::InstallFailureWriter([](const char* data, int size)
	{
		std::ofstream fs("failure_stack_dump.log", std::ios::app);
		std::string str = std::string(data, size);
		fs << std::string(data, size);
		fs.close();
		LOG(ERROR) << str;
	});
}

Logger::~Logger()
{
	google::FlushLogFiles(google::GLOG_INFO);
	google::ShutdownGoogleLogging();
}
