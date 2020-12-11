#pragma once

#include "imgui.h"

#include <string>
#include <vector>


namespace ImGui
{
	static auto vector_getter = [](void* vec, int idx, const char** out_text) {
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size()))
		{
			return false;
		}
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo(const char* label, int* current_item, std::vector<std::string>& values)
	{
		if (values.empty())
			return false;

		return Combo(label, current_item, vector_getter,
		             static_cast<void*>(&values),
		             static_cast<int>(values.size()));
	}

	bool ListBox(const char* label, int* current_item, std::vector<std::string>& values)
	{
		if (values.empty())
			return false;

		return ListBox(label, current_item, vector_getter,
		               static_cast<void*>(&values),
		               static_cast<int>(values.size()));
	}

} // namespace ImGui
