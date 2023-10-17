#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

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

    texture_2d fire_atlas(RESOURCE_DIR "textures/particles/fire.png", false);
    fire_atlas.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    fire_atlas.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fire_atlas.generate_mipmap();

    texture_2d explosion_atlas(RESOURCE_DIR "textures/particles/explosion.png", false);
    explosion_atlas.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    explosion_atlas.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    explosion_atlas.generate_mipmap();

    texture_2d smoke_atlas(RESOURCE_DIR "textures/particles/smoke.png", false);
    smoke_atlas.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    smoke_atlas.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    smoke_atlas.generate_mipmap();


    particle_props fire_particle_props;
    particle_props explosion_particle_props;
    particle_props smoke_particle_props;
    fire_particle_props.start_color = explosion_particle_props.start_color = smoke_particle_props.start_color = glm::vec4(1.0f);
    fire_particle_props.end_color = explosion_particle_props.end_color = smoke_particle_props.end_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    fire_particle_props.start_size = explosion_particle_props.start_size = smoke_particle_props.start_size = 0.1f;
    fire_particle_props.size_variation = explosion_particle_props.size_variation = smoke_particle_props.size_variation = 0.15f;
    fire_particle_props.end_size = explosion_particle_props.end_size = smoke_particle_props.end_size = 0.0f;
    fire_particle_props.life_time = explosion_particle_props.life_time = smoke_particle_props.life_time = 2.0f;
    fire_particle_props.velocity = explosion_particle_props.velocity = smoke_particle_props.velocity = glm::vec3(0.0f);
    fire_particle_props.velocity_variation = explosion_particle_props.velocity_variation = smoke_particle_props.velocity_variation = glm::vec3(0.2f);
    
    fire_particle_props.position = glm::vec3(-1.0f, 0.0f, 0.0f);
    explosion_particle_props.position = glm::vec3(0.0f);
    smoke_particle_props.position = glm::vec3(1.0f, 0.0f, 0.0f);


    particle_system fire_system(10000);
    fire_system.set_texture_atlas_dimension(8, 8);

    particle_system explosion_system(10000);
    explosion_system.set_texture_atlas_dimension(6, 8);

    particle_system smoke_system(10000);
    smoke_system.set_texture_atlas_dimension(8, 8);

    m_renderer.enable(GL_BLEND);
    m_renderer.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_renderer.set_clear_color(glm::vec4(1.0f));

    ImGuiIO& io = ImGui::GetIO();

    int32_t emited_count = 10;

    while (!glfwWindowShouldClose(m_window) && glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();

        if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS) {
            m_camera.is_fixed = !m_camera.is_fixed;

            glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

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
        
        for (size_t i = 0; i < emited_count; ++i) {
            fire_system.emit(fire_particle_props);
        }

        for (size_t i = 0; i < emited_count; ++i) {
            explosion_system.emit(explosion_particle_props);
        }

        for (size_t i = 0; i < emited_count; ++i) {
            smoke_system.emit(smoke_particle_props);
        }

        fire_system.update(io.DeltaTime, m_camera);
        explosion_system.update(io.DeltaTime, m_camera);
        smoke_system.update(io.DeltaTime, m_camera);

        particles_shader.uniform("u_proj_view", m_proj_settings.projection_mat * m_camera.get_view());

        particles_shader.uniform("u_atlas", fire_atlas, 0);
        m_renderer.render(GL_TRIANGLES, particles_shader, fire_system);

        particles_shader.uniform("u_atlas", explosion_atlas, 0);
        m_renderer.render(GL_TRIANGLES, particles_shader, explosion_system);

        particles_shader.uniform("u_atlas", smoke_atlas, 0);
        m_renderer.render(GL_TRIANGLES, particles_shader, smoke_system);

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
    ImGui_ImplOpenGL3_Init(glsl_version);
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
