#include "Window.h"

Window::Window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(800, 600, "Illinois Voxel Sandbox", nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow *Window::get_window() {
    return window_;
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(window_);
}

void Window::pollEvents() {
    glfwPollEvents();
}
