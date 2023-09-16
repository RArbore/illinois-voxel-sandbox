#include <algorithm>
#include <iostream> 

#include <external/glm/glm/gtc/matrix_transform.hpp>
#include <external/glm/glm/gtx/string_cast.hpp>>

#include "Camera.h"

static void mouse_callback(GLFWwindow *window, double x, double y) {
    Camera *camera =
        reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->handle_mouse(
            x, y,
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    }
}

Camera::Camera(std::shared_ptr<Window> window, const glm::vec3 &initial_pos,
               float pitch, float yaw, float speed, float sensitivity) 
    : window_{window}, origin_{initial_pos}, pitch_{pitch}, yaw_{yaw},
      speed_{speed}, sensitivity_{sensitivity} {
    auto glfw_window = window->get_window();
    glfwSetWindowUserPointer(glfw_window, reinterpret_cast<void *>(this));
    glfwSetCursorPosCallback(glfw_window, mouse_callback);

    up_ = glm::vec3(0.0f, 0.0f, 1.0f);

    float sin_pitch = sin(glm::radians(pitch_));
    float cos_pitch = cos(glm::radians(pitch_));
    float sin_yaw = sin(glm::radians(yaw_));
    float cos_yaw = cos(glm::radians(yaw_));
    front_ = glm::normalize(
        glm::vec3(cos_pitch * cos_yaw, cos_pitch * sin_yaw, sin_pitch));
}

std::shared_ptr<Camera>
create_camera(std::shared_ptr<Window> window, const glm::vec3 &initial_pos,
              float pitch, float yaw, float speed, float sensitivity)
{
    auto camera =
        std::shared_ptr<Camera>(new Camera(window, initial_pos, pitch, yaw, speed, sensitivity));
    return camera;
}

glm::vec3 Camera::get_right_vector() const {
    return glm::normalize(glm::cross(front_, up_));
}

void Camera::handle_keys(float delta_t) {
    auto glfw_window = window_->get_window();
    if (glfwGetKey(glfw_window, GLFW_KEY_W) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * front_;
        updated_since_last_frame = true;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_S) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * front_;
        updated_since_last_frame = true;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_A) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * get_right_vector();
        updated_since_last_frame = true;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_D) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * get_right_vector();
        updated_since_last_frame = true;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_Q) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * up_;
        updated_since_last_frame = true;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_E) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * up_;
        updated_since_last_frame = true;
    }
}

void Camera::handle_mouse(float x, float y, bool left_mouse_held) {
    static bool first_mouse = true;

    if (first_mouse) {
        last_x = x;
        last_y = y;
        first_mouse = false;
    }

    if (left_mouse_held) {
        float delta_x = last_x - x;
        float delta_y = y - last_y;

        yaw_ += delta_x * sensitivity_;
        pitch_ = std::clamp(pitch_ + delta_y * sensitivity_, -89.0f, 89.0f);

        float sin_pitch = sin(glm::radians(pitch_));
        float cos_pitch = cos(glm::radians(pitch_));
        float sin_yaw = sin(glm::radians(yaw_));
        float cos_yaw = cos(glm::radians(yaw_));
        front_ = glm::normalize(
            glm::vec3(cos_pitch * cos_yaw, cos_pitch * sin_yaw, sin_pitch));

        updated_since_last_frame = true;
    }

    last_x = x;
    last_y = y;
}

glm::mat4 Camera::get_view_matrix() const {
    return glm::lookAt(origin_, origin_ + front_, up_);
}

glm::mat4 Camera::get_view_inverse() const {
    return glm::inverse(get_view_matrix());
}

CameraUB Camera::get_uniform_buffer() const { 
    CameraUB uniform;
    uniform.view_inv = get_view_inverse();
    return uniform;
}