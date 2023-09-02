#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
  public:
    Window();
    ~Window();

    GLFWwindow *get_window();

    bool shouldClose();
    void pollEvents();

  private:
    GLFWwindow *window_;
};
