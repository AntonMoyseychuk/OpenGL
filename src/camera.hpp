#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <unordered_set>
#include <atomic>


class camera {
    public:
        camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float fov, float speed, float sensitivity, bool is_fixed = true);

        void rotate(float angle_radians, const glm::vec2& axis) noexcept;
        void move(const glm::vec3& offset) noexcept;

        const glm::vec3& get_forward() const noexcept;
        const glm::vec3& get_right() const noexcept;
        const glm::vec3& get_up() const noexcept;

        glm::mat4 get_view() const noexcept;

        void set_active(GLFWwindow* window) const noexcept;
        bool is_active() const noexcept;

        static void update_dt(float dt) noexcept;
        static camera* get_active_camera() noexcept;

    public:
        static void mouse_callback(GLFWwindow* window, double xpos, double ypos) noexcept;
        static void wheel_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
        
    private:
        void _recalculate_rotation(float delta_pitch, float delta_yaw) noexcept;

    private:
        static std::atomic<camera*> active_camera;
        static std::atomic<float> dt;

    public:
        glm::vec3 position;
        float speed;
        float sensitivity;
        float fov;
        bool is_fixed;

    private:
        glm::vec3 m_forward;
        glm::vec3 m_right;
        glm::vec3 m_up;

        glm::dvec2 m_last_cursor_pos;

    public:
        float m_pitch;
        float m_yaw;
    };