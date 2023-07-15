#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

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
    static void _framebuffer_resize_callback(GLFWwindow* window, int32_t width, int32_t height) noexcept;

private:
    struct glfw_deinitializer {
        void operator()(bool* is_glfw_initialized) noexcept;
    };

    static std::unique_ptr<bool, glfw_deinitializer> is_glfw_initialized;

private:
    std::string m_title;
    GLFWwindow* m_window = nullptr;

    float m_fov;
};