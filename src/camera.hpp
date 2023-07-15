#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class camera {
    public:
        camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float speed, float sensitivity);

        void rotate(float angle_radians, const glm::vec2& axis) noexcept;
        void move(const glm::vec3& offset) noexcept;

    #pragma region getters-setters
        float& get_speed() noexcept;
        float get_speed() const noexcept;

        float& get_sensitivity() noexcept;
        float get_sensitivity() const noexcept;

        glm::vec3& get_position() noexcept;
        const glm::vec3& get_position() const noexcept;
        const glm::vec3& get_forward() const noexcept;
        const glm::vec3& get_right() const noexcept;
        const glm::vec3& get_up() const noexcept;

        glm::mat4 get_view() const noexcept;
    #pragma endregion getters-setters

    private:
        glm::vec3 m_position;
        glm::vec3 m_forward;
        glm::vec3 m_right;
        glm::vec3 m_up;
        
        float m_radius;
        float m_speed;
        float m_sensitivity;

        float m_thi;
        float m_theta;
    };