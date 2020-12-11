#include "InputHandler.hpp"

#include <GLFW/glfw3.h>


void InputHandler::registerCallbacks(GLFWwindow* window)
{
	// Deposit a pointer to this class instance with GLFW for use in the callbacks below.
	glfwSetWindowUserPointer(window, this);

	// Register GLFW callbacks.
	glfwSetKeyCallback(window, &InputHandler::glfwKeyCallback);
	glfwSetScrollCallback(window, &InputHandler::glfwScrollCallback);
	glfwSetMouseButtonCallback(window, &InputHandler::glfwMouseButtonCallback);
}


void InputHandler::resetKeyboardInputs()
{
	alt_pressed = false;
	ctrl_pressed = false;
	space_pressed = false;
	backspace_pressed = false;
	tab_pressed = false;
	leftArrow_pressed = false;
	rightArrow_pressed = false;
	upArrow_pressed = false;
	downArrow_pressed = false;
	plus_pressed = false;
	minus_pressed = false;
	a_key_pressed = false;
	b_key_pressed = false;
	c_key_pressed = false;
	d_key_pressed = false;
	e_key_pressed = false;
	f_key_pressed = false;
	g_key_pressed = false;
	h_key_pressed = false;
	i_key_pressed = false;
	j_key_pressed = false;
	k_key_pressed = false;
	l_key_pressed = false;
	m_key_pressed = false;
	n_key_pressed = false;
	o_key_pressed = false;
	p_key_pressed = false;
	q_key_pressed = false;
	r_key_pressed = false;
	s_key_pressed = false;
	t_key_pressed = false;
	u_key_pressed = false;
	v_key_pressed = false;
	w_key_pressed = false;
	x_key_pressed = false;
	y_key_pressed = false;
	z_key_pressed = false;
	leftMouseButton_pressed = false;
	rightMouseButton_pressed = false;

	insert_key_pressed = false;
	delete_key_pressed = false;
	home_key_pressed = false;
	end_key_pressed = false;
	pageUp_key_pressed = false;
	pageDown_key_pressed = false;

	num0_pressed = false;
	num1_pressed = false;
	num2_pressed = false;
	num3_pressed = false;
	num4_pressed = false;
	num5_pressed = false;
	num6_pressed = false;
	num7_pressed = false;
	num8_pressed = false;
	num9_pressed = false;
	numDecimal_pressed = false;

	escape_pressed = false;
	f9_pressed = false;
	f10_pressed = false;
	f11_pressed = false;
	f12_pressed = false;
}


void InputHandler::resetMouseInputs()
{
	leftMouseButton_pressed = false;
	rightMouseButton_pressed = false;
	mouse_scrolled = false;
}


void InputHandler::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

	if (inputHandler == nullptr)
		return;

	// clang-format off
	if (key == GLFW_KEY_SPACE     && action == GLFW_PRESS)   { inputHandler->space_pressed = true; }
	if (key == GLFW_KEY_SPACE     && action == GLFW_RELEASE) { inputHandler->space_pressed = false; }
	if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)   { inputHandler->backspace_pressed = true; }
	if (key == GLFW_KEY_BACKSPACE && action == GLFW_RELEASE) { inputHandler->backspace_pressed = false; }
	if (key == GLFW_KEY_TAB       && action == GLFW_PRESS)   { inputHandler->tab_pressed = true; }
	if (key == GLFW_KEY_TAB       && action == GLFW_RELEASE) { inputHandler->tab_pressed = false; }
	
	if (key == GLFW_KEY_A && action == GLFW_PRESS)   { inputHandler->a_key_pressed = true; }
	if (key == GLFW_KEY_A && action == GLFW_RELEASE) { inputHandler->a_key_pressed = false; }
	if (key == GLFW_KEY_B && action == GLFW_PRESS)   { inputHandler->b_key_pressed = true; }
	if (key == GLFW_KEY_B && action == GLFW_RELEASE) { inputHandler->b_key_pressed = false; }
	if (key == GLFW_KEY_C && action == GLFW_PRESS)   { inputHandler->c_key_pressed = true; }
	if (key == GLFW_KEY_C && action == GLFW_RELEASE) { inputHandler->c_key_pressed = false; }
	if (key == GLFW_KEY_D && action == GLFW_PRESS)   { inputHandler->d_key_pressed = true; }
	if (key == GLFW_KEY_D && action == GLFW_RELEASE) { inputHandler->d_key_pressed = false; }
	if (key == GLFW_KEY_E && action == GLFW_PRESS)   { inputHandler->e_key_pressed = true; }
	if (key == GLFW_KEY_E && action == GLFW_RELEASE) { inputHandler->e_key_pressed = false; }
	if (key == GLFW_KEY_F && action == GLFW_PRESS)   { inputHandler->f_key_pressed = true; }
	if (key == GLFW_KEY_F && action == GLFW_RELEASE) { inputHandler->f_key_pressed = false; }
	if (key == GLFW_KEY_G && action == GLFW_PRESS)   { inputHandler->g_key_pressed = true; }
	if (key == GLFW_KEY_G && action == GLFW_RELEASE) { inputHandler->g_key_pressed = false; }
	if (key == GLFW_KEY_H && action == GLFW_PRESS)   { inputHandler->h_key_pressed = true; }
	if (key == GLFW_KEY_H && action == GLFW_RELEASE) { inputHandler->h_key_pressed = false; }
	if (key == GLFW_KEY_I && action == GLFW_PRESS)   { inputHandler->i_key_pressed = true; }
	if (key == GLFW_KEY_I && action == GLFW_RELEASE) { inputHandler->i_key_pressed = false; }
	if (key == GLFW_KEY_J && action == GLFW_PRESS)   { inputHandler->j_key_pressed = true; }
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) { inputHandler->j_key_pressed = false; }
	if (key == GLFW_KEY_K && action == GLFW_PRESS)   { inputHandler->k_key_pressed = true; }
	if (key == GLFW_KEY_K && action == GLFW_RELEASE) { inputHandler->k_key_pressed = false; }
	if (key == GLFW_KEY_L && action == GLFW_PRESS)   { inputHandler->l_key_pressed = true; }
	if (key == GLFW_KEY_L && action == GLFW_RELEASE) { inputHandler->l_key_pressed = false; }
	if (key == GLFW_KEY_M && action == GLFW_PRESS)   { inputHandler->m_key_pressed = true; }
	if (key == GLFW_KEY_M && action == GLFW_RELEASE) { inputHandler->m_key_pressed = false; }
	if (key == GLFW_KEY_N && action == GLFW_PRESS)   { inputHandler->n_key_pressed = true; }
	if (key == GLFW_KEY_N && action == GLFW_RELEASE) { inputHandler->n_key_pressed = false; }
	if (key == GLFW_KEY_O && action == GLFW_PRESS)   { inputHandler->o_key_pressed = true; }
	if (key == GLFW_KEY_O && action == GLFW_RELEASE) { inputHandler->o_key_pressed = false; }
	if (key == GLFW_KEY_P && action == GLFW_PRESS)   { inputHandler->p_key_pressed = true; }
	if (key == GLFW_KEY_P && action == GLFW_RELEASE) { inputHandler->p_key_pressed = false; }
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)   { inputHandler->q_key_pressed = true; }
	if (key == GLFW_KEY_Q && action == GLFW_RELEASE) { inputHandler->q_key_pressed = false; }
	if (key == GLFW_KEY_R && action == GLFW_PRESS)   { inputHandler->r_key_pressed = true; }
	if (key == GLFW_KEY_R && action == GLFW_RELEASE) { inputHandler->r_key_pressed = false; }
	if (key == GLFW_KEY_S && action == GLFW_PRESS)   { inputHandler->s_key_pressed = true; }
	if (key == GLFW_KEY_S && action == GLFW_RELEASE) { inputHandler->s_key_pressed = false; }
	if (key == GLFW_KEY_T && action == GLFW_PRESS)   { inputHandler->t_key_pressed = true; }
	if (key == GLFW_KEY_T && action == GLFW_RELEASE) { inputHandler->t_key_pressed = false; }
	if (key == GLFW_KEY_U && action == GLFW_PRESS)   { inputHandler->u_key_pressed = true; }
	if (key == GLFW_KEY_U && action == GLFW_RELEASE) { inputHandler->u_key_pressed = false; }
	if (key == GLFW_KEY_V && action == GLFW_PRESS)   { inputHandler->v_key_pressed = true; }
	if (key == GLFW_KEY_V && action == GLFW_RELEASE) { inputHandler->v_key_pressed = false; }
	if (key == GLFW_KEY_W && action == GLFW_PRESS)   { inputHandler->w_key_pressed = true; }
	if (key == GLFW_KEY_W && action == GLFW_RELEASE) { inputHandler->w_key_pressed = false; }
	if (key == GLFW_KEY_X && action == GLFW_PRESS)   { inputHandler->x_key_pressed = true; }
	if (key == GLFW_KEY_X && action == GLFW_RELEASE) { inputHandler->x_key_pressed = false; }
	if (key == GLFW_KEY_Y && action == GLFW_PRESS)   { inputHandler->y_key_pressed = true; }
	if (key == GLFW_KEY_Y && action == GLFW_RELEASE) { inputHandler->y_key_pressed = false; }
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)   { inputHandler->z_key_pressed = true; }
	if (key == GLFW_KEY_Z && action == GLFW_RELEASE) { inputHandler->z_key_pressed = false; }

	// used for color picking of framebuffer
	if ((key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) && action == GLFW_PRESS)   { inputHandler->alt_pressed = true; }
	if ((key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT) && action == GLFW_RELEASE) { inputHandler->alt_pressed = false; }
	if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS)   { inputHandler->ctrl_pressed = true; }
	if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_RELEASE) { inputHandler->ctrl_pressed = false; }
	if (key == GLFW_KEY_LEFT        && action == GLFW_PRESS)   { inputHandler->leftArrow_pressed = true; }
	if (key == GLFW_KEY_LEFT        && action == GLFW_RELEASE) { inputHandler->leftArrow_pressed = false; }
	if (key == GLFW_KEY_RIGHT       && action == GLFW_PRESS)   { inputHandler->rightArrow_pressed = true; }
	if (key == GLFW_KEY_RIGHT       && action == GLFW_RELEASE) { inputHandler->rightArrow_pressed = false; }
	if (key == GLFW_KEY_UP          && action == GLFW_PRESS)   { inputHandler->upArrow_pressed = true; }
	if (key == GLFW_KEY_UP          && action == GLFW_RELEASE) { inputHandler->upArrow_pressed = false; }
	if (key == GLFW_KEY_DOWN        && action == GLFW_PRESS)   { inputHandler->downArrow_pressed = true; }
	if (key == GLFW_KEY_DOWN        && action == GLFW_RELEASE) { inputHandler->downArrow_pressed = false; }
	if (key == GLFW_KEY_KP_ADD      && action == GLFW_PRESS)   { inputHandler->plus_pressed = true; }
	if (key == GLFW_KEY_KP_ADD      && action == GLFW_RELEASE) { inputHandler->plus_pressed = false; }
	if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS)   { inputHandler->minus_pressed = true; }
	if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_RELEASE) { inputHandler->minus_pressed = false; }

	if (key == GLFW_KEY_KP_0 && action == GLFW_PRESS)   { inputHandler->num0_pressed = true; }
	if (key == GLFW_KEY_KP_0 && action == GLFW_RELEASE) { inputHandler->num0_pressed = false; }
	if (key == GLFW_KEY_KP_1 && action == GLFW_PRESS)   { inputHandler->num1_pressed = true; }
	if (key == GLFW_KEY_KP_1 && action == GLFW_RELEASE) { inputHandler->num1_pressed = false; }
	if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS)   { inputHandler->num2_pressed = true; }
	if (key == GLFW_KEY_KP_2 && action == GLFW_RELEASE) { inputHandler->num2_pressed = false; }
	if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS)   { inputHandler->num3_pressed = true; }
	if (key == GLFW_KEY_KP_3 && action == GLFW_RELEASE) { inputHandler->num3_pressed = false; }
	if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS)   { inputHandler->num4_pressed = true; }
	if (key == GLFW_KEY_KP_4 && action == GLFW_RELEASE) { inputHandler->num4_pressed = false; }
	if (key == GLFW_KEY_KP_5 && action == GLFW_PRESS)   { inputHandler->num5_pressed = true; }
	if (key == GLFW_KEY_KP_5 && action == GLFW_RELEASE) { inputHandler->num5_pressed = false; }
	if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS)   { inputHandler->num6_pressed = true; }
	if (key == GLFW_KEY_KP_6 && action == GLFW_RELEASE) { inputHandler->num6_pressed = false; }
	if (key == GLFW_KEY_KP_7 && action == GLFW_PRESS)   { inputHandler->num7_pressed = true; }
	if (key == GLFW_KEY_KP_7 && action == GLFW_RELEASE) { inputHandler->num7_pressed = false; }
	if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS)   { inputHandler->num8_pressed = true; }
	if (key == GLFW_KEY_KP_8 && action == GLFW_RELEASE) { inputHandler->num8_pressed = false; }
	if (key == GLFW_KEY_KP_9 && action == GLFW_PRESS)   { inputHandler->num9_pressed = true; }
	if (key == GLFW_KEY_KP_9 && action == GLFW_RELEASE) { inputHandler->num9_pressed = false; }

	if (key == GLFW_KEY_KP_DECIMAL && action == GLFW_PRESS)   { inputHandler->numDecimal_pressed = true; }
	if (key == GLFW_KEY_KP_DECIMAL && action == GLFW_RELEASE) { inputHandler->numDecimal_pressed = false; }

	if (key == GLFW_KEY_INSERT    && action == GLFW_PRESS)   { inputHandler->insert_key_pressed = true; }
	if (key == GLFW_KEY_INSERT    && action == GLFW_RELEASE) { inputHandler->insert_key_pressed = false; }
	if (key == GLFW_KEY_DELETE    && action == GLFW_PRESS)   { inputHandler->delete_key_pressed = true; }
	if (key == GLFW_KEY_DELETE    && action == GLFW_RELEASE) { inputHandler->delete_key_pressed = false; }
	if (key == GLFW_KEY_HOME      && action == GLFW_PRESS)   { inputHandler->home_key_pressed = true; }
	if (key == GLFW_KEY_HOME      && action == GLFW_RELEASE) { inputHandler->home_key_pressed = false; }
	if (key == GLFW_KEY_END       && action == GLFW_PRESS)   { inputHandler->end_key_pressed = true; }
	if (key == GLFW_KEY_END       && action == GLFW_RELEASE) { inputHandler->end_key_pressed = false; }
	if (key == GLFW_KEY_PAGE_UP   && action == GLFW_PRESS)   { inputHandler->pageUp_key_pressed = true; }
	if (key == GLFW_KEY_PAGE_UP   && action == GLFW_RELEASE) { inputHandler->pageUp_key_pressed = false; }
	if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS)   { inputHandler->pageDown_key_pressed = true; }
	if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_RELEASE) { inputHandler->pageDown_key_pressed = false; }

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)   { inputHandler->escape_pressed  = true; }
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) { inputHandler->escape_pressed  = false; }

	if (key == GLFW_KEY_F9  && action == GLFW_PRESS)   { inputHandler->f9_pressed  = true; }
	if (key == GLFW_KEY_F9  && action == GLFW_RELEASE) { inputHandler->f9_pressed  = false; }
	if (key == GLFW_KEY_F10 && action == GLFW_PRESS)   { inputHandler->f10_pressed = true; }
	if (key == GLFW_KEY_F10 && action == GLFW_RELEASE) { inputHandler->f10_pressed = false; }
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)   { inputHandler->f11_pressed = true; }
	if (key == GLFW_KEY_F11 && action == GLFW_RELEASE) { inputHandler->f11_pressed = false; }
	if (key == GLFW_KEY_F12 && action == GLFW_PRESS)   { inputHandler->f12_pressed = true; }
	if (key == GLFW_KEY_F12 && action == GLFW_RELEASE) { inputHandler->f12_pressed = false; }
	// clang-format on
}


void InputHandler::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

	if (inputHandler == nullptr)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS /*|| action == GLFW_REPEAT*/) inputHandler->leftMouseButton_pressed = true;
		if (action == GLFW_RELEASE) inputHandler->leftMouseButton_pressed = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)   inputHandler->rightMouseButton_pressed = true;
		if (action == GLFW_RELEASE) inputHandler->rightMouseButton_pressed = false;
	}
}


void InputHandler::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	InputHandler* inputHandler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));

	if (inputHandler == nullptr)
		return;

	inputHandler->mouse_scrolled = true;
	inputHandler->mouseScrollOffset = Eigen::Vector2f(xoffset, yoffset);
}
