#pragma once

#include <memory>

#include <external/glm/glm/glm.hpp>

#include "Window.h"

struct CameraUB {
    glm::mat4 view_inv;
    int frames_since_update;
};

class Camera {
  public:
    Camera(std::shared_ptr<Window> window, const glm::vec3 &initial_pos,
           float pitch, float yaw, float speed, float sensitivity);

    float speed_;
    float sensitivity_;

    void handle_keys(float delta_t);
    void handle_mouse(float x, float y, bool left_mouse_held);
    void mark_rendered() { frames_since_update_++; };

    void recompute_vectors();
    void reset_frames_since_update();

    glm::mat4 get_view_inverse() const;
    CameraUB get_uniform_buffer() const;
    glm::vec3 get_front() const;
    glm::vec3 get_position() const;

    std::shared_ptr<Window> window_;
    glm::vec3 origin_, world_up_, front_, right_;
    float pitch_, yaw_;
    float last_x, last_y;
    int frames_since_update_;
};

std::shared_ptr<Camera> create_camera(std::shared_ptr<Window> window,
                                      const glm::vec3 &initial_pos, float pitch,
                                      float yaw, float speed,
                                      float sensitivity);
