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
) noexcept {
    this->position = position;
    this->speed = speed;
    this->sensitivity = sensitivity;
    this->fov = fov;
    this->is_fixed = is_fixed;

    this->m_forward = glm::normalize(look_at - position);
    this->m_right = glm::normalize(glm::cross(up, -m_forward));
    this->m_up = glm::cross(-m_forward, m_right);

    this->m_pitch = glm::degrees(glm::angle(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    this->m_yaw = glm::degrees(glm::angle(m_forward, glm::vec3(1.0f, 0.0f, 0.0f)));
    if (glm::sign(m_forward.z) < 0.0f) {
        this->m_yaw = 360.0f - this->m_yaw;
    }
}

void camera::rotate(float angle_degrees, const glm::vec3& axis) noexcept {
    // m_pitch = glm::clamp(m_pitch + angle_degrees * axis.x, 1.0f, 179.0f);
    // m_yaw += angle_degrees * axis.y;
    // _recalculate_rotation();
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

void camera::invert_pitch() noexcept {
    m_pitch = glm::abs(m_pitch - 179.0f);
    _recalculate_rotation();
}

void camera::update_dt(float dt) noexcept {
    camera::dt.store(dt);
}

void camera::mouse_callback(double xpos, double ypos) noexcept {
    const glm::dvec2 last_pos = m_last_cursor_pos;

    m_last_cursor_pos.x = xpos;
    m_last_cursor_pos.y = ypos;

    if (!is_fixed) {
        float xoffset = (xpos - last_pos.x) * sensitivity * dt.load();
        float yoffset = (ypos - last_pos.y) * sensitivity * dt.load(); 

        m_pitch = glm::clamp(m_pitch + yoffset, 1.0f, 179.0f);
        m_yaw += xoffset;

        _recalculate_rotation();
    }
}

void camera::wheel_scroll_callback(double yoffset) noexcept {
    if (!is_fixed) {
        fov = glm::clamp(fov - static_cast<float>(yoffset) * sensitivity, 1.0f, 179.0f);
    }
}

void camera::_recalculate_rotation() noexcept {
    const float y = glm::cos(glm::radians(m_pitch));
    const float x = glm::cos(glm::radians(m_yaw)) * glm::sin(glm::radians(m_pitch));
    const float z = glm::sin(glm::radians(m_yaw)) * glm::sin(glm::radians(m_pitch));
    m_forward = glm::normalize(glm::vec3(x, y, z));

    m_right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -m_forward));
    m_up = glm::normalize(glm::cross(-m_forward, m_right));
}
