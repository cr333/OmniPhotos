#pragma once

#include <algorithm>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <vector>


namespace std
{
//==== STL Fixes ===================================================================================

// Visual Studio 2010 does not provide all 9 overloads of the std::to_string function mandated by the C++11 standard.
// Solution: provide the missing overloads here. <http://stackoverflow.com/a/10666695/72470>
#if _MSC_VER == 1600

	inline string to_string(int val)           { return std::to_string(static_cast<long long>(val)); }
	inline string to_string(unsigned val)      { return std::to_string(static_cast<long long>(val)); }
	inline string to_string(long val)          { return std::to_string(static_cast<long long>(val)); }
	inline string to_string(unsigned long val) { return std::to_string(static_cast<long long>(val)); }
	inline string to_string(float val)         { return std::to_string(static_cast<long double>(val)); }
	inline string to_string(double val)        { return std::to_string(static_cast<long double>(val)); }

#endif


	//==== STL Extensions ==============================================================================


	//---- Splitting a string --------------------------------------------------------------------------
	// The following two functions are adapted from <http://stackoverflow.com/a/236803/72470>.

	// Splits the string str by a delimiter and returns the items.
	inline vector<string>& split(const string& str, char delimiter, vector<string>& items)
	{
		stringstream ss(str);
		string item;

		while (getline(ss, item, delimiter))
			items.push_back(item);

		return items;
	}

	// Splits the string str by a delimiter and returns the items.
	inline vector<string> split(const string& str, char delimiter)
	{
		vector<string> items;
		split(str, delimiter, items);
		return items;
	}


	//---- Trimming a string ---------------------------------------------------------------------------
	// The following three functions are adapted from <http://stackoverflow.com/a/1798170/72470>.

	// Returns a copy of the string with leading characters removed.
	inline string ltrim(const string& str, const string& whitespace = " \t")
	{
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == string::npos)
			return ""; // no content

		return str.substr(strBegin);
	}

	// Returns a copy of the string with trailing characters removed.
	inline string rtrim(const string& str, const string& whitespace = " \t")
	{
		const auto strEnd = str.find_last_not_of(whitespace);
		if (strEnd == string::npos)
			return ""; // no content

		return str.substr(0, strEnd + 1);
	}

	// Returns a copy of the string with leading and trailing characters removed.
	inline string trim(const string& str, const string& whitespace = " \t")
	{
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == string::npos)
			return ""; // no content

		const auto strEnd = str.find_last_not_of(whitespace);
		const auto strRange = strEnd - strBegin + 1;

		return str.substr(strBegin, strRange);
	}


	//---- String upper/lower-case conversions ---------------------------------------------------------
	// The following four functions are adapted from <http://stackoverflow.com/a/735259/72470>.

	namespace
	{
		inline string::value_type tolower_char(string::value_type ch)
		{
			return tolower<typename string::value_type>(ch, locale());
		}

		inline string::value_type toupper_char(string::value_type ch)
		{
			return toupper<typename string::value_type>(ch, locale());
		}
	} // end of anonymous namespace

	// Returns a copy of the string with upper-case characters converted to lower case. This operation is locale specific.
	inline string lower(const string& str)
	{
		string result;
		result.reserve(str.length());

		// Could alternatively use ::tolower, but char may be signed and negative values can cause problems.
		transform(begin(str), end(str), back_inserter(result), tolower_char);
		return result;
	}

	// Returns a copy of the string with lower-case characters converted to upper case. This operation is locale specific.
	inline string upper(const string& str)
	{
		string result;
		result.reserve(str.length());

		// Could alternatively use ::toupper, but char may be signed and negative values can cause problems.
		transform(begin(str), end(str), back_inserter(result), toupper_char);
		return result;
	}

} // namespace std
