#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


// Convenience macro to raise a runtime exception with filename and line number.
#define RUNTIME_EXCEPTION(message) \
	ExceptionWrapper<std::runtime_error>::throwException(__FILE__, __LINE__, message)

// Convenience macro to raise an out-of-range exception with filename and line number.
#define OUT_OF_RANGE_EXCEPTION(message) \
	ExceptionWrapper<std::out_of_range>::throwException(__FILE__, __LINE__, message)


template <class Exception_Type>
class ExceptionWrapper : public Exception_Type
{
public:
	// Throws an exception augmented with filename and line number.
	static void throwException(const char* file, const int line, const std::string& message)
	{
		std::stringstream s;
		s << "Exception in '" << file << "' (line " << line << "):" << std::endl
		  << "Details: " << message << std::endl;

		throw ExceptionWrapper(s.str());
	}

	virtual ~ExceptionWrapper() noexcept {}

private:
	// Default constructor is private, so this class cannot be created except through throwException(...).
	ExceptionWrapper() :
	    Exception_Type("Unknown exception.")
	{
	}

	ExceptionWrapper(const std::string& message) :
	    Exception_Type(message)
	{
	}
};
