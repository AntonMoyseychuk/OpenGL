#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

camera::camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float speed, float sensitivity)
    : position(position),
        m_forward(glm::normalize(look_at - position)),
        m_right(glm::normalize(glm::cross(up, -m_forward))),
        m_up(glm::cross(-m_forward, m_right)),
        speed(speed),
        sensitivity(sensitivity),
        m_thi(glm::degrees(glm::angle(glm::normalize(glm::vec3(m_forward.x, 0.0f, m_forward.z)), glm::vec3(0.0f, 0.0f, 1.0f)))),
        m_theta(glm::degrees(glm::angle(m_forward, glm::vec3(0.0f, 1.0f, 0.0f))))
{
}

    
void camera::rotate(float angle_radians, const glm::vec2& axis) noexcept {
    m_thi += axis.y * glm::degrees(angle_radians) * sensitivity;
    m_theta += axis.x * glm::degrees(angle_radians) * sensitivity;

    const auto forward_x = std::sinf(glm::radians(m_theta)) * std::sinf(glm::radians(m_thi));
    const auto forward_y = std::cosf(glm::radians(m_theta));
    const auto forward_z = std::sinf(glm::radians(m_theta)) * std::cosf(glm::radians(m_thi));
    m_forward = glm::normalize(glm::vec3(forward_x, forward_y, forward_z));

    m_right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -m_forward);
    m_up = glm::cross(-m_forward, m_right);
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