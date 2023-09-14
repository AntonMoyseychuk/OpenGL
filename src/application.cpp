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

#include "random.hpp"

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
    shader textured_shader(RESOURCE_DIR "shaders/csm/textured.vert", RESOURCE_DIR "shaders/csm/textured.frag");
    shader flat_color_shader(RESOURCE_DIR "shaders/csm/flat_color.vert", RESOURCE_DIR "shaders/csm/flat_color.frag");
    shader shadowmap_shader(RESOURCE_DIR "shaders/csm/shadowmap.vert", RESOURCE_DIR "shaders/csm/shadowmap.frag");

    framebuffer shadowmap_fbo;
    shadowmap_fbo.create();

    texture_2d shadowmap(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);
    shadowmap.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    shadowmap.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    shadowmap.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    shadowmap.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    shadowmap.set_tex_parameter(GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4(1.0f)));

    shadowmap_fbo.attach(GL_DEPTH_ATTACHMENT, 0, shadowmap);
    shadowmap_fbo.set_draw_buffer(GL_NONE);
    shadowmap_fbo.set_read_buffer(GL_NONE);

    model::texture_load_config config;
    config.flip_on_load = true;
    config.use_gamma = false;
    config.generate_mipmap = true;
    config.mag_filter = GL_LINEAR;
    config.min_filter = GL_LINEAR;
    config.wrap_s = GL_REPEAT;
    config.wrap_t = GL_REPEAT;
    model backpack(RESOURCE_DIR "models/backpack/backpack.obj", config);
    
    model cube(RESOURCE_DIR "models/cube/cube.obj", model::texture_load_config());
    
    uv_sphere sphere(40, 40);
    
    texture_2d wall_albedo(RESOURCE_DIR "textures/wall_albedo_map.jpg", true, false, texture_2d::variety::DIFFUSE);
    wall_albedo.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    wall_albedo.set_tex_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    wall_albedo.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    wall_albedo.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    texture_2d wall_normal(RESOURCE_DIR "textures/wall_normal_map.jpg", true, false, texture_2d::variety::NORMAL);
    wall_normal.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    wall_normal.set_tex_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    wall_normal.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    wall_normal.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    mesh plane(
        std::vector<mesh::vertex>{
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
            { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
        }, 
        { 0, 2, 1, 0, 3, 2 }
    );
    plane.add_texture(std::move(wall_albedo));
    plane.add_texture(std::move(wall_normal));
    
    glm::vec3 light_direction(1.0f, -1.0f, -1.0f); glm::vec3 light_color(1.0f); float intensity = 1.0f;
    glm::vec3 floor_position(0.0f, -5.0f, 0.0f);
    glm::vec3 sphere_position(-10.0f, -3.0f, 10.0f);
    glm::vec3 cube_position(10.0f, -3.0f, -10.0f);
    glm::vec3 backpack_position(4.0f, -3.0f, -2.0f);
    float far = 45.0f;
    float dir_light_dist = 15.0f;

    const auto render_scene = [&](const shader* shadowmap_sh = nullptr) -> void {
        const glm::mat4 light_proj = glm::ortho(-17.0f, 17.0f, -17.0f, 17.0f, 0.1f, far);
        const glm::mat4 light_view = glm::lookAt(-glm::normalize(light_direction) * dir_light_dist, glm::normalize(light_direction), glm::vec3(0.0f, 1.0f, 0.0f));
        
        if (shadowmap_sh != nullptr) {
            shadowmap_sh->uniform("u_projection", light_proj);
            shadowmap_sh->uniform("u_view", light_view);
            shadowmap_sh->uniform("u_model", 
                glm::translate(glm::mat4(1.0f), floor_position) *
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * 
                glm::scale(glm::mat4(1.0f), glm::vec3(30.0f))
            );
            m_renderer.render(GL_TRIANGLES, *shadowmap_sh, plane);

            shadowmap_sh->uniform("u_model", glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));
            m_renderer.render(GL_TRIANGLES, *shadowmap_sh, backpack);

            shadowmap_sh->uniform("u_model", glm::translate(glm::mat4(1.0f), sphere_position));
            m_renderer.render(GL_TRIANGLES, *shadowmap_sh, sphere.mesh);

            shadowmap_sh->uniform("u_model", glm::translate(glm::mat4(1.0f), cube_position));
            m_renderer.render(GL_TRIANGLES, *shadowmap_sh, cube);
        } else {
            textured_shader.uniform("u_light.direction", glm::normalize(light_direction));
            textured_shader.uniform("u_light.color", light_color);
            textured_shader.uniform("u_light.intensity", intensity);
            textured_shader.uniform("u_light.shadowmap", 10);
            shadowmap.bind(10);
            textured_shader.uniform("u_material.shininess", 16.0f);
            textured_shader.uniform("u_camera_position", m_camera.position);
            textured_shader.uniform("u_light_space", light_proj * light_view);
            textured_shader.uniform("u_projection", m_proj_settings.projection_mat);
            textured_shader.uniform("u_view", m_camera.get_view());
            textured_shader.uniform("u_model", 
                glm::translate(glm::mat4(1.0f), floor_position) *
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * 
                glm::scale(glm::mat4(1.0f), glm::vec3(30.0f))
            );
            m_renderer.render(GL_TRIANGLES, textured_shader, plane);

            textured_shader.uniform("u_material.shininess", 64.0f);
            textured_shader.uniform("u_model", glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));
            m_renderer.render(GL_TRIANGLES, textured_shader, backpack);

            flat_color_shader.uniform("u_light.direction", glm::normalize(light_direction));
            flat_color_shader.uniform("u_light.color", light_color);
            flat_color_shader.uniform("u_light.intensity", intensity);
            flat_color_shader.uniform("u_light.shadowmap", 10);
            flat_color_shader.uniform("u_material.shininess", 64.0f);
            flat_color_shader.uniform("u_material.albedo_color", glm::vec3(1.0f));
            flat_color_shader.uniform("u_camera_position", m_camera.position);
            flat_color_shader.uniform("u_light_space", light_proj * light_view);
            flat_color_shader.uniform("u_projection", m_proj_settings.projection_mat);
            flat_color_shader.uniform("u_view", m_camera.get_view());

            flat_color_shader.uniform("u_model", glm::translate(glm::mat4(1.0f), sphere_position));
            m_renderer.render(GL_TRIANGLES, flat_color_shader, sphere.mesh);

            flat_color_shader.uniform("u_model", glm::translate(glm::mat4(1.0f), cube_position));
            m_renderer.render(GL_TRIANGLES, flat_color_shader, cube);
        }        
    };

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

        shadowmap_fbo.bind();
        m_renderer.clear(GL_DEPTH_BUFFER_BIT);
        render_scene(&shadowmap_shader);

        framebuffer::bind_default();
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_scene();
        

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
                
            if (ImGui::NewLine(), ImGui::ColorEdit3("background color", glm::value_ptr(m_clear_color))) {
                m_renderer.set_clear_color(m_clear_color.r, m_clear_color.g, m_clear_color.b, 1.0f);
            }

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Light");
            ImGui::DragFloat3("direction", glm::value_ptr(light_direction), 0.1f);
            ImGui::ColorEdit3("color", glm::value_ptr(light_color));
            ImGui::DragFloat("intensity", &intensity, 0.1f);
            ImGui::DragFloat("distance", &dir_light_dist, 0.1f);
        ImGui::End();

        ImGui::Begin("Texture");
            ImGui::Image(
                (void*)(intptr_t)shadowmap.get_id(), ImVec2(shadowmap.get_width(), shadowmap.get_height()), ImVec2(0, 1), ImVec2(1, 0)
            );
        ImGui::End();

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
