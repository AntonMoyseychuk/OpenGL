#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.hpp"

#include <string>
#include <memory>

class application {
public:
    static application& get(const std::string_view& title, uint32_t width, uint32_t height) noexcept;

    void run() noexcept;

private:
    application(const std::string_view& title, uint32_t width, uint32_t height);
    ~application();

private:
    void _destroy() noexcept;

private:
    void _imgui_init(const char* glsl_version) const noexcept;
    void _imgui_shutdown() const noexcept;

    void _imgui_frame_begin() const noexcept;
    void _imgui_frame_end() const noexcept;

private:
    static void _window_resize_callback(GLFWwindow* window, int width, int height) noexcept;
    static void _mouse_callback(GLFWwindow *window, double xpos, double ypos) noexcept;
    static void _mouse_wheel_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept;

private:
    struct projection_settings {
        float aspect;
        float near;
        float far;
        uint32_t width;
        uint32_t height;

        glm::mat4 projection_mat;
    };

private:
    struct glfw_deinitializer {
        void operator()(bool* is_glfw_initialized) noexcept;
    };

    static std::unique_ptr<bool, glfw_deinitializer> is_glfw_initialized;

private:
    std::string m_title;
    GLFWwindow* m_window = nullptr;

    projection_settings m_proj_settings;

    camera m_camera;
    glm::vec3 m_clear_color = glm::vec3(0.0f);

    bool m_wireframed = false;
};
