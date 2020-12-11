#pragma once

#include "3rdParty/Eigen.hpp"

struct GLFWwindow;


class InputHandler
{
public:
	InputHandler()
	{
		mouseScrollOffset = Eigen::Vector2f(0, 0);
	}

	~InputHandler() = default;


	double deltaTime = 0; //glfw delay in one app::run()

	bool mouse_scrolled = false;
	Eigen::Vector2f mouseScrollOffset;

	bool alt_pressed = false;
	bool ctrl_pressed = false;
	bool space_pressed = false;
	bool backspace_pressed = false;
	bool tab_pressed = false;
	bool leftArrow_pressed = false;
	bool rightArrow_pressed = false;
	bool upArrow_pressed = false;
	bool downArrow_pressed = false;
	bool plus_pressed = false;
	bool minus_pressed = false;
	bool a_key_pressed = false;
	bool b_key_pressed = false;
	bool c_key_pressed = false;
	bool d_key_pressed = false;
	bool e_key_pressed = false;
	bool f_key_pressed = false;
	bool g_key_pressed = false;
	bool h_key_pressed = false;
	bool i_key_pressed = false;
	bool j_key_pressed = false;
	bool k_key_pressed = false;
	bool l_key_pressed = false;
	bool m_key_pressed = false;
	bool n_key_pressed = false;
	bool o_key_pressed = false;
	bool p_key_pressed = false;
	bool q_key_pressed = false;
	bool r_key_pressed = false;
	bool s_key_pressed = false;
	bool t_key_pressed = false;
	bool u_key_pressed = false;
	bool v_key_pressed = false;
	bool w_key_pressed = false;
	bool x_key_pressed = false;
	bool y_key_pressed = false;
	bool z_key_pressed = false;
	bool leftMouseButton_pressed = false;
	bool rightMouseButton_pressed = false;

	bool insert_key_pressed = false;
	bool delete_key_pressed = false;
	bool home_key_pressed = false;
	bool end_key_pressed = false;
	bool pageUp_key_pressed = false;
	bool pageDown_key_pressed = false;

	bool num0_pressed = false;
	bool num1_pressed = false;
	bool num2_pressed = false;
	bool num3_pressed = false;
	bool num4_pressed = false;
	bool num5_pressed = false;
	bool num6_pressed = false;
	bool num7_pressed = false;
	bool num8_pressed = false;
	bool num9_pressed = false;
	bool numDecimal_pressed = false;

	bool escape_pressed = false;
	bool f9_pressed = false;
	bool f10_pressed = false;
	bool f11_pressed = false;
	bool f12_pressed = false;

	// Register this handler for keyboard and mouse callbacks.
	void registerCallbacks(GLFWwindow* window);

	// Resets all keyboard inputs.
	void resetKeyboardInputs();

	// Resets all mouse inputs.
	void resetMouseInputs();

	// GLFW Callbacks.
	static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};
