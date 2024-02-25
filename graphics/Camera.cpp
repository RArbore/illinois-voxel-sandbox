#include <algorithm>
#include <iostream>

#include <external/glm/glm/gtc/matrix_transform.hpp>
#include <external/glm/glm/gtx/string_cast.hpp>

#include "Camera.h"

static void mouse_callback(GLFWwindow *window, double x, double y) {
    Camera *camera =
        reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
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

    world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    recompute_vectors();

    frames_since_update_ = 0;
}

std::shared_ptr<Camera> create_camera(std::shared_ptr<Window> window,
                                      const glm::vec3 &initial_pos, float pitch,
                                      float yaw, float speed,
                                      float sensitivity) {
    auto camera = std::shared_ptr<Camera>(
        new Camera(window, initial_pos, pitch, yaw, speed, sensitivity));
    return camera;
}

void Camera::recompute_vectors() {
    float sin_pitch = sin(glm::radians(pitch_));
    float cos_pitch = cos(glm::radians(pitch_));
    float sin_yaw = sin(glm::radians(yaw_));
    float cos_yaw = cos(glm::radians(yaw_));
    front_ = glm::normalize(
        glm::vec3(cos_pitch * sin_yaw, sin_pitch, cos_pitch * cos_yaw));

    right_ = glm::normalize(glm::cross(front_, world_up_));
}

void Camera::reset_frames_since_update() { frames_since_update_ = 0; }

void Camera::handle_keys(float delta_t) {
    auto glfw_window = window_->get_window();
    if (glfwGetKey(glfw_window, GLFW_KEY_W) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * front_;
        frames_since_update_ = 0;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_S) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * front_;
        frames_since_update_ = 0;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_A) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * right_;
        frames_since_update_ = 0;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_D) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * right_;
        frames_since_update_ = 0;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_Q) == GLFW_PRESS) {
        origin_ += delta_t * speed_ * world_up_;
        frames_since_update_ = 0;
    }

    if (glfwGetKey(glfw_window, GLFW_KEY_E) == GLFW_PRESS) {
        origin_ -= delta_t * speed_ * world_up_;
        frames_since_update_ = 0;
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

        recompute_vectors();

        frames_since_update_ = 0;
    }

    last_x = x;
    last_y = y;
}

glm::mat4 Camera::get_view_inverse() const {
    const glm::vec3 up = glm::normalize(glm::cross(right_, front_));

    glm::mat4 view_inverse{0};

    view_inverse[0][0] = right_[0];
    view_inverse[0][1] = right_[1];
    view_inverse[0][2] = right_[2];

    view_inverse[1][0] = up[0];
    view_inverse[1][1] = up[1];
    view_inverse[1][2] = up[2];

    view_inverse[2][0] = front_[0];
    view_inverse[2][1] = front_[1];
    view_inverse[2][2] = front_[2];

    view_inverse[3][0] = origin_[0];
    view_inverse[3][1] = origin_[1];
    view_inverse[3][2] = origin_[2];
    view_inverse[3][3] = 1.0f;

    return view_inverse;
}

CameraUB Camera::get_uniform_buffer() const {
    CameraUB uniform;
    uniform.view_inv = get_view_inverse();
    uniform.frames_since_update = frames_since_update_;
    return uniform;
}

glm::vec3 Camera::get_front() const { return front_; }

glm::vec3 Camera::get_position() const { return origin_; }
