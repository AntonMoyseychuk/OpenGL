#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class camera {
    public:
        camera(const glm::vec3& position, const glm::vec3& look_at, const glm::vec3& up, float speed, float sensitivity);

        void rotate(float angle_radians, const glm::vec2& axis) noexcept;
        void move(const glm::vec3& offset) noexcept;

    #pragma region getters-setters
        const glm::vec3& get_forward() const noexcept;
        const glm::vec3& get_right() const noexcept;
        const glm::vec3& get_up() const noexcept;

        glm::mat4 get_view() const noexcept;
    #pragma endregion getters-setters

    public:
        glm::vec3 position;
        float speed;
        float sensitivity;

    private:
        glm::vec3 m_forward;
        glm::vec3 m_right;
        glm::vec3 m_up;

        float m_thi;
        float m_theta;
    };