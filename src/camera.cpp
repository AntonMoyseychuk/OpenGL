#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "application.hpp"

std::atomic<float> camera::dt = 0.0f;


camera::camera(const glm::vec3 &position, const glm::vec3 &look_at, const glm::vec3 &up, float fov, float speed, float sensitivity, bool is_fixed) {
    create(position, look_at, up, fov, speed, sensitivity, is_fixed);
}

void camera::create(const glm::vec3 &position, const glm::vec3 &look_at, const glm::vec3 &up, 
    float fov, float speed, float sensitivity, bool is_fixed
) const noexcept {
    camera* self = const_cast<camera*>(this);

    self->position = position;
    self->m_forward = glm::normalize(look_at - position);
    self->m_right = glm::normalize(glm::cross(up, -m_forward));
    self->m_up = glm::cross(-m_forward, m_right);
    self->speed = speed;
    self->sensitivity = sensitivity;
    self->fov = fov;
    self->is_fixed = is_fixed;
    self->m_pitch = glm::degrees(glm::angle(glm::normalize(glm::vec3(m_forward.x, 0.0f, m_forward.z)), glm::vec3(0.0f, 0.0f, 1.0f)));
    self->m_yaw = glm::degrees(glm::angle(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void camera::rotate(float angle_degrees, const glm::vec2& axis) noexcept {
    _recalculate_rotation(axis.x * angle_degrees, axis.y * angle_degrees);
}

void camera::move(const glm::vec3 &offset) noexcept {
    position += offset * speed;
}

const glm::vec3& camera::get_forward() const noexcept {
    return m_forward;
}

const glm::vec3& camera::get_right() const noexcept {
    return m_right;
}

const glm::vec3& camera::get_up() const noexcept {
    return m_up;
}
    
glm::mat4 camera::get_view() const noexcept {
    return glm::lookAt(position, position + m_forward, m_up);
}

void camera::update_dt(float dt) noexcept {
    camera::dt.store(dt);
}

void camera::mouse_callback(double xpos, double ypos) noexcept {
    const glm::dvec2 last_pos = m_last_cursor_pos;

    m_last_cursor_pos.x = xpos;
    m_last_cursor_pos.y = ypos;

    if (!is_fixed) {
        double xoffset = (xpos - last_pos.x) * sensitivity * dt.load();
        double yoffset = (ypos - last_pos.y) * sensitivity * dt.load(); 

        _recalculate_rotation(yoffset, xoffset);
    }
}

void camera::wheel_scroll_callback(double yoffset) noexcept {
    if (!is_fixed) {
        fov = glm::clamp(fov - static_cast<float>(yoffset) * sensitivity, 1.0f, 179.0f);
    }
}

void camera::_recalculate_rotation(float delta_pitch, float delta_yaw) noexcept {
    m_pitch = glm::clamp(m_pitch + delta_pitch, 91.0f, 269.0f);
    m_yaw += delta_yaw;

    const auto forward_x = std::cosf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch));
    const auto forward_y = std::sinf(glm::radians(m_pitch));
    const auto forward_z = std::sinf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch));
    m_forward = glm::normalize(glm::vec3(forward_x, forward_y, forward_z));

    m_right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -m_forward));
    m_up = glm::normalize(glm::cross(-m_forward, m_right));
}
