#pragma once

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
  public:
    Window();
    ~Window();

    GLFWwindow *get_window();

    bool should_close();
    void poll_events();

  private:
    GLFWwindow *window_;
};

std::shared_ptr<Window> create_window();
