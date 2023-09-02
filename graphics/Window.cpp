#include "Window.h"

Window::Window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(800, 800, "Illinois Voxel Sandbox", nullptr,
                               nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow *Window::get_window() { return window_; }

bool Window::shouldClose() { return glfwWindowShouldClose(window_); }

void Window::pollEvents() { glfwPollEvents(); }
