#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include "debug.hpp"

#include "shader.hpp"
#include "texture.hpp"
#include "cubemap.hpp"
#include "model.hpp"
#include "framebuffer.hpp"

#include "light/spot_light.hpp"
#include "light/point_light.hpp"
#include "light/directional_light.hpp"

#include "uv_sphere.hpp"

#include <algorithm>
#include <map>

std::unique_ptr<bool, application::glfw_deinitializer> application::is_glfw_initialized(new bool(glfwInit() == GLFW_TRUE));

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title), m_camera(glm::vec3(0.0f, 1.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 18.0f, 10.0f)
{
    const char* glfw_error_msg = nullptr;
    ASSERT(is_glfw_initialized != nullptr && *is_glfw_initialized, "GLFW error", 
        (glfwGetError(&glfw_error_msg), glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error"));

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

// #ifdef _DEBUG
//     glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
// #endif

    m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
    ASSERT(m_window != nullptr, "GLFW error", (_destroy(), glfwGetError(&glfw_error_msg), glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error"));
    glfwSetWindowUserPointer(m_window, this);

    glfwMakeContextCurrent(m_window);
    ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "GLAD error", "failed to initialize GLAD");
    glfwSwapInterval(1);

// #ifdef _DEBUG
//     glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//     glDebugMessageCallback(MessageCallback, nullptr);
// #endif

    glfwSetCursorPosCallback(m_window, &application::_mouse_callback);
    glfwSetScrollCallback(m_window, &application::_mouse_wheel_scroll_callback);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 200.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);

    m_renderer.enable(GL_DEPTH_TEST);
    m_renderer.depth_func(GL_LEQUAL);
    
    // m_renderer.enable(GL_BLEND);
    // m_renderer.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _imgui_init("#version 430 core");
}

void application::run() noexcept {
    shader scene_shader(RESOURCE_DIR "shaders/hdr_and_bloom/scene.vert", RESOURCE_DIR "shaders/hdr_and_bloom/scene.frag");
    shader blur_shader(RESOURCE_DIR "shaders/hdr_and_bloom/blur.vert", RESOURCE_DIR "shaders/hdr_and_bloom/blur.frag");
    shader bloom_tonemapping_shader(RESOURCE_DIR "shaders/hdr_and_bloom/tonemapping.vert", RESOURCE_DIR "shaders/hdr_and_bloom/tonemapping.frag");
    shader light_source_shader(RESOURCE_DIR "shaders/hdr_and_bloom/light_source.vert", RESOURCE_DIR "shaders/hdr_and_bloom/light_source.frag");

    framebuffer hdr_fbo;
    hdr_fbo.create();

    texture_2d color_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    color_buffer.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    color_buffer.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    hdr_fbo.attach(GL_COLOR_ATTACHMENT0, 0, color_buffer);

    texture_2d bright_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    bright_buffer.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    bright_buffer.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    hdr_fbo.attach(GL_COLOR_ATTACHMENT1, 0, bright_buffer);

    renderbuffer depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);
    assert(hdr_fbo.attach(GL_DEPTH_ATTACHMENT, depth_buffer));

    uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    OGL_CALL(glDrawBuffers(2, attachments));

    texture_2d pinpong_color_buffers[2];
    framebuffer pinpong_fbos[2];
    for (size_t i = 0; i < 2; ++i) {
        pinpong_fbos[i].create();

        pinpong_color_buffers[i].create(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
        pinpong_color_buffers[i].set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        pinpong_color_buffers[i].set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        pinpong_color_buffers[i].set_tex_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        pinpong_color_buffers[i].set_tex_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        assert(pinpong_fbos[i].attach(GL_COLOR_ATTACHMENT0, 0, pinpong_color_buffers[i]));
    }

    model::texture_load_config config;
    config.flip_on_load = true;
    config.format = GL_RGB;
    config.generate_mipmap = true;
    config.internal_format = GL_RGB;
    config.level = 0;
    config.mag_filter = GL_LINEAR;
    config.min_filter = GL_LINEAR;
    config.target = GL_TEXTURE_2D;
    config.type = GL_UNSIGNED_BYTE;
    config.wrap_s = GL_REPEAT;
    config.wrap_t = GL_REPEAT;
    model backpack(RESOURCE_DIR "models/backpack/backpack.obj", config);
    
    uv_sphere sphere(40, 40);
    
    mesh plane(
        std::vector<mesh::vertex>{
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
            { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { {1.0f,  1.0f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { {1.0f, -1.0f, 0.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
        }, 
        {0, 2, 1, 0, 3, 2}
    );

    glm::vec3 backpack_position(0.0f), light_position(0.0f, 0.0f, 2.0f);
    glm::vec3 light_color(1.0f); float intensity = 1.0f, exposure = 1.0f;
    bool use_normal_mapping = true;

    ImGuiIO& io = ImGui::GetIO();
    while (!glfwWindowShouldClose(m_window) && glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();

        m_camera.update_dt(io.DeltaTime);
        if (!m_camera.is_fixed) {
            if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                m_camera.move(m_camera.get_up() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                m_camera.move(-m_camera.get_up() * io.DeltaTime);
            }
            if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
                m_camera.move(m_camera.get_forward() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
                m_camera.move(-m_camera.get_forward() * io.DeltaTime);
            }
            if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
                m_camera.move(m_camera.get_right() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
                m_camera.move(-m_camera.get_right() * io.DeltaTime);
            }
        }

        hdr_fbo.bind();
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene_shader.uniform("u_projection", m_proj_settings.projection_mat);
        scene_shader.uniform("u_view", m_camera.get_view());
        scene_shader.uniform("u_model", glm::translate(glm::mat4(1.0f), backpack_position));
        scene_shader.uniform("u_light.position", light_position);
        scene_shader.uniform("u_light.color", light_color);
        scene_shader.uniform("u_light.intensity", intensity);
        scene_shader.uniform("u_light.constant", 1.0f);
        scene_shader.uniform("u_light.linear", 0.7f);
        scene_shader.uniform("u_light.quadratic", 1.8f);
        scene_shader.uniform("u_material.shininess", 16.0f);
        scene_shader.uniform("u_camera_position", m_camera.position);
        scene_shader.uniform("u_use_normal_mapping", use_normal_mapping);
        m_renderer.render(GL_TRIANGLES, scene_shader, backpack);

        light_source_shader.uniform("u_projection", m_proj_settings.projection_mat);
        light_source_shader.uniform("u_view", m_camera.get_view());
        light_source_shader.uniform("u_model", glm::translate(glm::mat4(1.0f), light_position) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f)));
        light_source_shader.uniform("u_color", light_color);
        m_renderer.render(GL_TRIANGLES, light_source_shader, sphere.mesh);

        bool horizontal = true, first_iter = true;
        constexpr size_t amount = 10;
        for (size_t i = 0; i < amount; ++i) {
            pinpong_fbos[horizontal].bind();
            
            blur_shader.uniform("u_horizontal", horizontal);
            
            blur_shader.uniform("u_scene", 0);
            if (first_iter) {
                first_iter = false;
                bright_buffer.bind(0);
            } else {
                pinpong_color_buffers[!horizontal].bind(0);
            }

            m_renderer.render(GL_TRIANGLES, blur_shader, plane);
            horizontal = !horizontal;
        }

        framebuffer::bind_default();
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        color_buffer.bind(0);
        pinpong_color_buffers[!horizontal].bind(1);
        bloom_tonemapping_shader.uniform("u_hdr_buffer", 0);
        bloom_tonemapping_shader.uniform("u_bloom_buffer", 1);
        bloom_tonemapping_shader.uniform("u_exposure", exposure);
        m_renderer.render(GL_TRIANGLES, bloom_tonemapping_shader, plane);

    #if 1 //UI
        _imgui_frame_begin();
        ImGui::Begin("Information");
            ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION)); ImGui::NewLine();
                
            if (ImGui::Checkbox("wireframe mode", &m_wireframed)) {
                m_renderer.polygon_mode(GL_FRONT_AND_BACK, (m_wireframed ? GL_LINE : GL_FILL));
            }

            if (ImGui::Checkbox("cull face", &m_cull_face)) {
                m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);
            }

            ImGui::Checkbox("use normal mapping", &use_normal_mapping);
                
            if (ImGui::NewLine(), ImGui::ColorEdit3("background color", glm::value_ptr(m_clear_color))) {
                m_renderer.set_clear_color(m_clear_color.r, m_clear_color.g, m_clear_color.b, 1.0f);
            }

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Light");
            ImGui::DragFloat3("position", glm::value_ptr(light_position), 0.1f);
            ImGui::ColorEdit3("color", glm::value_ptr(light_color));
            ImGui::DragFloat("intensity", &intensity, 0.1f);
            ImGui::DragFloat("exposure", &exposure, 0.1f);
        ImGui::End();

        // ImGui::Begin("Texture");
        //     ImGui::Image(
        //         (void*)(intptr_t)bright_buffer.get_id(), ImVec2(bright_buffer.get_width(), bright_buffer.get_height()), ImVec2(0, 1), ImVec2(1, 0)
        //     );
        // ImGui::End();

        ImGui::Begin("Camera");
            ImGui::DragFloat3("position", glm::value_ptr(m_camera.position), 0.1f);
            if (ImGui::DragFloat("speed", &m_camera.speed, 0.1f) || 
                ImGui::DragFloat("sensitivity", &m_camera.sensitivity, 0.1f)
            ) {
                m_camera.speed = std::clamp(m_camera.speed, 1.0f, std::numeric_limits<float>::max());
                m_camera.sensitivity = std::clamp(m_camera.sensitivity, 0.1f, std::numeric_limits<float>::max());
            }
            if (ImGui::SliderFloat("field of view", &m_camera.fov, 1.0f, 179.0f)) {
                _window_resize_callback(m_window, m_proj_settings.width, m_proj_settings.height);
            }

            if (ImGui::Checkbox("fixed (Press \'1\')", &m_camera.is_fixed)) {
                glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
            } else if (glfwGetKey(m_window, GLFW_KEY_1)) {
                m_camera.is_fixed = !m_camera.is_fixed;

                glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        ImGui::End();
        _imgui_frame_end();
    #endif

        glfwSwapBuffers(m_window);
    }
}

application::~application() {
    _imgui_shutdown();
    _destroy();
}

void application::_destroy() noexcept {
    glfwDestroyWindow(m_window);
}

void application::_imgui_init(const char *glsl_version) const noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430 core");
}

void application::_imgui_shutdown() const noexcept {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void application::_imgui_frame_begin() const noexcept {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void application::_imgui_frame_end() const noexcept {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void application::glfw_deinitializer::operator()(bool *is_glfw_initialized) noexcept {
    glfwTerminate();
    if (is_glfw_initialized != nullptr) {
        *is_glfw_initialized = false;
    }
}

void application::_window_resize_callback(GLFWwindow *window, int width, int height) noexcept {
    application* app = static_cast<application*>(glfwGetWindowUserPointer(window));
    ASSERT(app != nullptr, "", "application instance is not set");

    app->m_renderer.viewport(app->m_proj_settings.x, app->m_proj_settings.y, width, height);

    app->m_proj_settings.width = width;
    app->m_proj_settings.height = height;
    
    app->m_proj_settings.aspect = static_cast<float>(width) / height;
    
    const camera& active_camera = app->m_camera;

    app->m_proj_settings.projection_mat = glm::perspective(glm::radians(active_camera.fov), app->m_proj_settings.aspect, app->m_proj_settings.near, app->m_proj_settings.far);
}

void application::_mouse_callback(GLFWwindow *window, double xpos, double ypos) noexcept {
    application* app = static_cast<application*>(glfwGetWindowUserPointer(window));
    ASSERT(app != nullptr, "", "application instance is not set");

    app->m_camera.mouse_callback(xpos, ypos);
}

void application::_mouse_wheel_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept {
    application* app = static_cast<application*>(glfwGetWindowUserPointer(window));
    ASSERT(app != nullptr, "", "application instance is not set");

    app->m_camera.wheel_scroll_callback(yoffset);
    
    _window_resize_callback(window, app->m_proj_settings.width, app->m_proj_settings.height);
}
