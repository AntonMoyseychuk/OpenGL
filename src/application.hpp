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
    void _init_imgui(const char* glsl_version) const noexcept;
    void _shutdown_imgui() const noexcept;

    void _imgui_frame_begin() const noexcept;
    void _imgui_frame_end() const noexcept;

private:
    struct framebuf_resize_controller {
    public:
        static framebuf_resize_controller& get() noexcept;
        static void on_resize(GLFWwindow* window, int32_t width, int32_t height) noexcept;

    private:
        framebuf_resize_controller() = default;
        framebuf_resize_controller(float fov, float aspect, float near, float far);

    public:
        float fov;
        float aspect;
        float near;
        float far;
        uint32_t width;
        uint32_t height;
        glm::mat4 projection;
    };

    static framebuf_resize_controller& framebuffer;

private:
    struct glfw_deinitializer {
        void operator()(bool* is_glfw_initialized) noexcept;
    };

    static std::unique_ptr<bool, glfw_deinitializer> is_glfw_initialized;

private:
    std::string m_title;
    GLFWwindow* m_window = nullptr;

    camera m_camera;
    bool m_wireframed = false;
    bool m_fixed_camera = false;
};