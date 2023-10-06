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

    const glm::vec3 camera_position(0.0f, 0.0f, 25.0f);
    m_camera.create(camera_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 300.0f, 10.0f);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 100.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);
}

void application::run() noexcept {
    shader particles_shader(RESOURCE_DIR "shaders/particles/particles.vert", RESOURCE_DIR "shaders/particles/particles.frag");

    const float width = m_proj_settings.width;
    const float height = m_proj_settings.height;

    const float left = -m_proj_settings.aspect;
    const float right = m_proj_settings.aspect;
    const float bottom = -1.0f;
    const float top = 1.0f;

    const float bounds_width = right - left;
    const float bounds_height = top - bottom;
    particles_shader.uniform("u_proj_view", glm::ortho(left, right, bottom, top, -1.0f, 100.0f) * m_camera.get_view());
    particle_props props;
    props.start_color = { 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f };
	props.end_color = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
	props.start_size = 0.1f, props.size_variation = 0.3f, props.end_size = 0.0f;
	props.life_time = 1.0f;
	props.velocity = { 0.0f, 0.0f };
	props.velocity_variation = { 3.0f, 1.0f };
	props.position = { 0.0f, 0.0f };

    particle_system p_system(1000);

    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(m_window) && glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();

        // m_camera.update_dt(io.DeltaTime);
        // if (!m_camera.is_fixed) {
        //     if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        //         m_camera.move(m_camera.get_up() * io.DeltaTime);
        //     } else if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        //         m_camera.move(-m_camera.get_up() * io.DeltaTime);
        //     }
        //     if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
        //         m_camera.move(m_camera.get_forward() * io.DeltaTime);
        //     } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
        //         m_camera.move(-m_camera.get_forward() * io.DeltaTime);
        //     }
        //     if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
        //         m_camera.move(m_camera.get_right() * io.DeltaTime);
        //     } else if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
        //         m_camera.move(-m_camera.get_right() * io.DeltaTime);
        //     }
        // }

        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        int32_t mouse_state = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);
        if (mouse_state == GLFW_PRESS || mouse_state == GLFW_REPEAT) {
            double x, y;
            glfwGetCursorPos(m_window, &x, &y);

            x = (x / width) * bounds_width - bounds_width * 0.5f;
            y = bounds_height * 0.5f - (y / height) * bounds_height;
            props.position = { x + m_camera.position.x, y + m_camera.position.y };

            for (size_t i = 0; i < 5; ++i) {
                p_system.emit(props);
            }
        }

        p_system.update(io.DeltaTime);

        m_renderer.render(GL_TRIANGLES, particles_shader, p_system);

        _imgui_frame_begin();
        ImGui::Begin("Settings");
            ImGui::ColorEdit4("Birth Color", glm::value_ptr(props.start_color));
            ImGui::ColorEdit4("Death Color", glm::value_ptr(props.end_color));
            ImGui::DragFloat("Life Time", &props.life_time, 0.1f, 0.0f, 1000.0f);
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

    #if 0 //UI
    #if 1        
        ImGui::Begin("Water Framebuffers");
            ImGui::SliderInt(debug_water_fbos_index == 0 ? "refract" : "reflect", &debug_water_fbos_index, 0, 1);
            const uint32_t fbo_width = debug_water_fbos_index == 0 ? refract_color_buffer.get_width() : reflect_color_buffer.get_width();
            const uint32_t fbo_height = debug_water_fbos_index == 0 ? refract_color_buffer.get_height() : reflect_color_buffer.get_height();
            ImGui::Image((void*)(intptr_t)(debug_water_fbos_index == 0 ? refract_color_buffer.get_id() : reflect_color_buffer.get_id()), 
                ImVec2(fbo_width, fbo_height), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();

        // ImGui::Begin("Refract Depth");
        //     ImGui::Image((void*)(intptr_t)refract_depth_buffer.get_id(), ImVec2(fbo_width, fbo_height), ImVec2(0, 1), ImVec2(1, 0));
        // ImGui::End();

        // ImGui::Begin("Terrain");
        //     ImGui::DragFloat("height scale", &terrain.height_scale, 0.001f, 0.01f, std::numeric_limits<float>::max());
        //     ImGui::DragFloat("world scale", &terrain.world_scale, 0.1f, 0.1f, std::numeric_limits<float>::max());

        //     if (ImGui::Button("regenerate")) {
        //         terrain.create(RESOURCE_DIR "textures/terrain/height_map.png", terrain.world_scale, terrain.height_scale);

        //         texture_2d surface(RESOURCE_DIR "textures/terrain/terrain_surface.png");
        //         terrain.mesh.add_texture(std::move(surface));
        //     }
        // ImGui::End();
    #endif

        ImGui::Begin("Water");
            ImGui::DragFloat("wave strength", &wave_strength, 0.001f, 0.001f, std::numeric_limits<float>::max());
            ImGui::DragFloat("wave speed", &wave_speed, 0.001f, 0.001f, std::numeric_limits<float>::max());
            ImGui::DragFloat("shininess", &water_shininess, 0.1f, 0.1f, std::numeric_limits<float>::max());
            ImGui::DragFloat("specular strength", &water_specular_strength, 0.1f, 0.1f, std::numeric_limits<float>::max());
        ImGui::End();

        ImGui::Begin("Skybox");
            ImGui::DragFloat("speed", &skybox_speed, 0.001f, 0.0f, std::numeric_limits<float>::max());
        ImGui::End();

        ImGui::Begin("Light");
            ImGui::ColorEdit3("color", glm::value_ptr(light_color));
            ImGui::Checkbox("debug cascade", &cascade_debug_mode);
        ImGui::End();

        ImGui::Begin("Depth Texture");
            ImGui::SliderInt("cascade index", &debug_cascade_index, 0, csm_shadowmap.shadowmaps.size() - 1);
            const uint32_t csm_width = csm_shadowmap.shadowmaps[debug_cascade_index].get_width();
            const uint32_t csm_height = csm_shadowmap.shadowmaps[debug_cascade_index].get_height();
            ImGui::Text("Resolution [%d X %d]", csm_width, csm_height);
            ImGui::Image((void*)(intptr_t)csm_shadowmap.shadowmaps[debug_cascade_index].get_id(), 
                ImVec2(csm_width, csm_height), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();

        ImGui::Begin("Fog");
            ImGui::ColorEdit3("color", glm::value_ptr(fog_color));
            ImGui::DragFloat("density", &fog_density, 0.0001f, 0.0f, std::numeric_limits<float>::max(), "%.5f");
            ImGui::DragFloat("gradient", &fog_gradient, 0.01f, 0.0f, std::numeric_limits<float>::max());
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
    #endif
        _imgui_frame_end();

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
