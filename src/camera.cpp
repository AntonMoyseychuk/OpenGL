#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "application.hpp"

std::atomic<camera*> camera::active_camera = nullptr;
std::atomic<float> camera::dt = 0.0f;

camera::camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float fov, float speed, float sensitivity, bool is_fixed)
    : position(position),
        m_forward(glm::normalize(look_at - position)),
        m_right(glm::normalize(glm::cross(up, -m_forward))),
        m_up(glm::cross(-m_forward, m_right)),
        speed(speed),
        sensitivity(sensitivity),
        fov(fov),
        is_fixed(is_fixed),
        m_pitch(glm::degrees(glm::angle(glm::normalize(glm::vec3(m_forward.x, 0.0f, m_forward.z)), glm::vec3(0.0f, 0.0f, 1.0f)))),
        m_yaw(glm::degrees(glm::angle(m_forward, glm::vec3(0.0f, 1.0f, 0.0f))))
{
}

    
void camera::rotate(float angle_radians, const glm::vec2& axis) noexcept {
    _recalculate_rotation(axis.y * glm::degrees(angle_radians) * sensitivity, axis.x * glm::degrees(angle_radians) * sensitivity);
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

void camera::set_active(GLFWwindow* window) const noexcept {
    camera* self = const_cast<camera*>(this);
    
    active_camera.store(self);
    glfwGetCursorPos(window, &self->m_last_cursor_pos.x, &self->m_last_cursor_pos.y);
}

bool camera::is_active() const noexcept {
    return active_camera == this;
}

void camera::mouse_callback(GLFWwindow *window, double xpos, double ypos) noexcept {
    if (active_camera.load() != nullptr) {
        const glm::dvec2 last_pos = active_camera.load()->m_last_cursor_pos;

        active_camera.load()->m_last_cursor_pos.x = xpos;
        active_camera.load()->m_last_cursor_pos.y = ypos;

        if (!active_camera.load()->is_fixed) {
            double xoffset = (xpos - last_pos.x) * active_camera.load()->sensitivity * camera::dt.load();
            double yoffset = (ypos - last_pos.y) * active_camera.load()->sensitivity * camera::dt.load(); 

            active_camera.load()->_recalculate_rotation(yoffset, xoffset);
        }
    }
}

void camera::wheel_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept {    
    if (active_camera.load() != nullptr) {
        if (!active_camera.load()->is_fixed) {
            active_camera.load()->fov = glm::clamp(active_camera.load()->fov - (float)yoffset * active_camera.load()->sensitivity, 1.0f, 179.0f);
        }
    }
}

void camera::update_dt(float dt) noexcept {
    camera::dt.store(dt);
}

camera *camera::get_active_camera() noexcept {
    return active_camera;
}

void camera::_recalculate_rotation(float delta_pitch, float delta_yaw) noexcept {
    m_pitch += delta_pitch;
    m_yaw += delta_yaw;

    m_pitch = glm::clamp(m_pitch, 91.0f, 269.0f);

    const auto forward_x = std::cosf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch));
    const auto forward_y = std::sinf(glm::radians(m_pitch));
    const auto forward_z = std::sinf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch));
    m_forward = glm::normalize(glm::vec3(forward_x, forward_y, forward_z));

    m_right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -m_forward);
    m_up = glm::cross(-m_forward, m_right);
}
