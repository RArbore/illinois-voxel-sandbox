#include "Window.h"

Window::Window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ =
        glfwCreateWindow(1600, 900, "Illinois Voxel Sandbox", nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow *Window::get_window() { return window_; }

bool Window::should_close() { return glfwWindowShouldClose(window_); }

void Window::poll_events() { glfwPollEvents(); }

std::shared_ptr<Window> create_window() { return std::make_shared<Window>(); }
