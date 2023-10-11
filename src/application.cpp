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

#include "csm.hpp"
#include "uv_sphere.hpp"
#include "terrain.hpp"

#include "particle_system.hpp"

#include "random.hpp"

#include <algorithm>
#include <map>

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title)
{
    if (glfwInit() != GLFW_TRUE) {
        const char* glfw_error_msg = nullptr;
        glfwGetError(&glfw_error_msg);

        LOG_ERROR("GLFW error", glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error");
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

// #ifdef _DEBUG
//     glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
// #endif

    m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        const char* glfw_error_msg = nullptr;
        glfwGetError(&glfw_error_msg);

        LOG_ERROR("GLFW error", glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error");
        _destroy();
        exit(-1);
    }
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

    _imgui_init("#version 460 core");

    m_renderer.enable(GL_DEPTH_TEST);
    m_renderer.depth_func(GL_LEQUAL);

    const glm::vec3 camera_position(0.0f, 0.0f, 8.0f);
    m_camera.create(camera_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 3.0f, 10.0f);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 100.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);
}

void application::run() noexcept {
    shader particles_shader(RESOURCE_DIR "shaders/particles/particles.vert", RESOURCE_DIR "shaders/particles/particles.frag");

    texture_2d particle_texture(RESOURCE_DIR "textures/particles/particle_atlas2.png", false);
    particle_texture.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    particle_texture.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    particle_texture.generate_mipmap();

    particles_shader.uniform("u_sprite", particle_texture, 0);

    particle_props props;
    props.start_color = glm::vec4(1.0f);
	props.end_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	props.start_size = 0.1f, props.size_variation = 0.15f, props.end_size = 0.0f;
	props.life_time = 2.0f;
	props.velocity = glm::vec3(0.0f);
	props.velocity_variation = glm::vec3(0.2f);
	props.position = glm::vec3(0.0f);

    particle_system p_system(10000);
    p_system.set_texture_atlas_dimension(6, 8);

    m_renderer.enable(GL_BLEND);
    m_renderer.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float width = m_proj_settings.width;
    const float height = m_proj_settings.height;

    const float left = -m_proj_settings.aspect;
    const float right = m_proj_settings.aspect;
    const float bottom = -1.0f;
    const float top = 1.0f;

    const float bounds_width = right - left;
    const float bounds_height = top - bottom;

    ImGuiIO& io = ImGui::GetIO();

    int32_t emited_count = 15;

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

        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // double x, y;
            // glfwGetCursorPos(m_window, &x, &y);
            //
            // x = (x / width) * bounds_width - bounds_width * 0.5f;
            // y = bounds_height * 0.5f - (y / height) * bounds_height;
            // props.position = { x + m_camera.position.x, y + m_camera.position.y };

            for (size_t i = 0; i < emited_count; ++i) {
                p_system.emit(props);
            }
        }

        p_system.update(io.DeltaTime, m_camera);

        particles_shader.uniform("u_proj_view", m_proj_settings.projection_mat * m_camera.get_view());
        m_renderer.render(GL_TRIANGLES, particles_shader, p_system);

    #if 1 //UI
        _imgui_frame_begin();
        ImGui::Begin("particles");
            ImGui::ColorEdit4("birth color", glm::value_ptr(props.start_color));
            ImGui::ColorEdit4("death color", glm::value_ptr(props.end_color));
            ImGui::DragFloat3("velocity variation", glm::value_ptr(props.velocity_variation), 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("start size", &props.start_size, 0.001f, 0.0f, 1000.0f);
            ImGui::DragFloat("end size", &props.end_size, 0.001f, 0.0f, 1000.0f);
            ImGui::DragFloat("size variation", &props.size_variation, 0.001f, 0.0f, 1000.0f);
            ImGui::DragFloat("life time", &props.life_time, 0.01f, 0.0f, 1000.0f);
            ImGui::DragInt("emission power", &emited_count, 1, 0, 1000);
        ImGui::End();
        
        ImGui::Begin("Information");
            ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION)); ImGui::NewLine();
                
            if (ImGui::Checkbox("wireframe mode", &m_wireframed)) {
                m_renderer.polygon_mode(GL_FRONT_AND_BACK, (m_wireframed ? GL_LINE : GL_FILL));
            }

            if (ImGui::Checkbox("cull face", &m_cull_face)) {
                m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);
            }
                
            if (ImGui::NewLine(), ImGui::ColorEdit4("background color", glm::value_ptr(m_clear_color))) {
                m_renderer.set_clear_color(m_clear_color);
            }

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Camera");
            ImGui::DragFloat3("position", glm::value_ptr(m_camera.position), 0.1f);
            ImGui::DragFloat("speed", &m_camera.speed, 0.1f, 1.0f, std::numeric_limits<float>::max());
            ImGui::DragFloat("sensitivity", &m_camera.sensitivity, 0.1f, 0.1f, std::numeric_limits<float>::max());

            if (ImGui::SliderFloat("field of view", &m_camera.fov, 1.0f, 179.0f)) {
                _window_resize_callback(m_window, m_proj_settings.width, m_proj_settings.height);
            }

            if (ImGui::Checkbox("fixed (Press \'F\')", &m_camera.is_fixed)) {
                glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
            } else if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS) {
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
    // NOTE: causes invalid end of program since it't called before all OpenGL objects are deleted
    // glfwDestroyWindow(m_window);
    // glfwTerminate();
}

void application::_imgui_init(const char *glsl_version) const noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
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

void application::_window_resize_callback(GLFWwindow *window, int width, int height) noexcept {
    application* app = static_cast<application*>(glfwGetWindowUserPointer(window));
    ASSERT(app != nullptr, "", "application instance is not set");

    if (width != 0 && height != 0) {
        app->m_renderer.viewport(app->m_proj_settings.x, app->m_proj_settings.y, width, height);

        app->m_proj_settings.width = width;
        app->m_proj_settings.height = height;
        
        app->m_proj_settings.aspect = static_cast<float>(width) / height;
        
        const camera& active_camera = app->m_camera;
        
        app->m_proj_settings.projection_mat = glm::perspective(glm::radians(active_camera.fov), app->m_proj_settings.aspect, app->m_proj_settings.near, app->m_proj_settings.far);
    }
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
