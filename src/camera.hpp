#pragma once
#include <glm/glm.hpp>

#include <unordered_set>
#include <atomic>


class camera {
    public:
        camera() = default;
        camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float fov, float speed, float sensitivity, bool is_fixed = true);

        void create(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float fov, float speed, float sensitivity, bool is_fixed = true) noexcept;

        void rotate(float angle_degrees, const glm::vec2& axis) noexcept;
        void move(const glm::vec3& offset) noexcept;

        const glm::vec3& get_forward() const noexcept;
        const glm::vec3& get_right() const noexcept;
        const glm::vec3& get_up() const noexcept;

        glm::mat4 get_view() const noexcept;

    public:
        static void update_dt(float dt) noexcept;

    public:
        void mouse_callback(double xpos, double ypos) noexcept;
        void wheel_scroll_callback(double yoffset) noexcept;
        
    private:
        void _recalculate_rotation(float delta_pitch, float delta_yaw) noexcept;


    public:
        glm::vec3 position;
        float speed;
        float sensitivity;
        float fov;
        bool is_fixed;

    private:
        static std::atomic<float> dt;

    private:
        glm::vec3 m_forward;
        glm::vec3 m_right;
        glm::vec3 m_up;

        glm::dvec2 m_last_cursor_pos;

        float m_pitch;
        float m_yaw;
    };