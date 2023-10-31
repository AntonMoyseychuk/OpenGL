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

    const glm::vec3 camera_position(0.0f, 0.0f, 15.0f);
    m_camera.create(camera_position, camera_position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 5.0f, 10.0f);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 100.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);
}

void application::run() noexcept {
    ImGuiIO& io = ImGui::GetIO();

    shader terrain_shader(RESOURCE_DIR "shaders/grass/terrain.vert", RESOURCE_DIR "shaders/grass/terrain.frag");
    shader grass_shader(RESOURCE_DIR "shaders/grass/grass.vert", RESOURCE_DIR "shaders/grass/grass.frag");

    constexpr const glm::vec3 terrain_min(-10.0f, 0.0f, -10.0f);
    constexpr const glm::vec3 terrain_max( 10.0f, 0.0f,  10.0f);
    constexpr const float terrain_width = glm::abs(terrain_max.x - terrain_min.x);
    constexpr const float terrain_depth = glm::abs(terrain_max.z - terrain_min.z);
    mesh terrain(
        std::vector<mesh::vertex> {
            {                                   terrain_min, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(         0.0f,          0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(terrain_min.x, 0.0f, terrain_max.z), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(         0.0f, terrain_max.z), glm::vec3(1.0f, 0.0f, 0.0f) },
            {                                   terrain_max, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(terrain_max.x, terrain_max.z), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(terrain_max.x, 0.0f, terrain_min.z), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(terrain_max.x,          0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
        },
        std::vector<uint32_t> {
            0, 1, 2,
            0, 2, 3
        }
    );
    texture_2d terrain_texture(RESOURCE_DIR "textures/terrain/grass_tile.png");
    terrain_texture.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    terrain_texture.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    terrain_texture.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    terrain_texture.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    terrain_texture.generate_mipmap();
    terrain_shader.uniform("u_texture", terrain_texture, 0);

    const glm::mat4 terrain_transform_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -5.0f, 0.0f)); 
    const glm::mat3 terrain_normal_matrix = glm::mat3(glm::transpose(glm::inverse(terrain_transform_matrix)));

    constexpr const uint32_t grass_in_raw = 300;
    constexpr float dx = terrain_width / grass_in_raw, dz = terrain_depth / grass_in_raw;


    std::vector<glm::mat4> grass_transform_matrix((grass_in_raw + 1) * (grass_in_raw + 1) * 3);
    for (uint32_t z = 0; z <= grass_in_raw; ++z) {
        for (uint32_t x = 0; x <= grass_in_raw * 3; x += 3) {  
            std::array<glm::vec3, 3> positions = {
                glm::clamp(glm::vec3(terrain_min.x + (x + random(-0.5f, 0.5f)) * dx, 0.0f, terrain_min.z + (z + random(-0.5f, 0.5f)) * dz), 
                    terrain_min, 
                    terrain_max
                ),
                glm::clamp(glm::vec3(terrain_min.x + (x + 1 + random(-0.5f, 0.5f)) * dx, 0.0f, terrain_min.z + (z + random(-0.5f, 0.5f)) * dz), 
                    terrain_min, 
                    terrain_max
                ),
                glm::clamp(glm::vec3(terrain_min.x + (x + 2 + random(-0.5f, 0.5f)) * dx, 0.0f, terrain_min.z + (z + random(-0.5f, 0.5f)) * dz), 
                    terrain_min, 
                    terrain_max
                )
            };

            std::array<glm::mat4, positions.size()> translations;
            for (size_t i = 0; i < translations.size(); ++i) {
                translations[i] = glm::translate(glm::mat4(1.0f), positions[i]);
            }

            std::array<glm::mat4, positions.size()> rotation_scale;
            for (size_t i = 0; i < rotation_scale.size(); ++i) {
                rotation_scale[i] = glm::rotate(glm::mat4(1.0f), glm::radians(random(1.0f, 89.0f)), glm::vec3(0.0f, 1.0f, 0.0f))
                    * glm::scale(glm::mat4(1.0f), glm::vec3(random(0.1f, 1.0f), random(0.1f, 1.0f), 1.0f));
            }

            for (size_t i = 0; i < 3; ++i) {
                grass_transform_matrix[x + i + z * (grass_in_raw + 1)] = terrain_transform_matrix * translations[i] * rotation_scale[i];
            }
        }
    }
    buffer grass_transform_buffer(GL_SHADER_STORAGE_BUFFER, 
        grass_transform_matrix.size() * sizeof(grass_transform_matrix[0]), sizeof(grass_transform_matrix[0]), GL_DYNAMIC_DRAW, grass_transform_matrix.data());

    model grass(RESOURCE_DIR "models/terrain/grass_2.obj", std::nullopt);

    glm::vec3 wind_direction(1.0f, 0.0f, 0.0f);
    float wind_strength = 0.5f;

    glm::vec3 top_color(0.13f, 0.56f, 0.11f);
    glm::vec3 bottom_color(0.05f, 0.11f, 0.01f);

    while (!glfwWindowShouldClose(m_window) && glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();

        m_camera.update_dt(io.DeltaTime);
        if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS) {
            m_camera.is_fixed = !m_camera.is_fixed;

            glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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

        terrain_shader.uniform("u_view_matrix", m_camera.get_view());
        terrain_shader.uniform("u_projection_matrix", m_proj_settings.projection_mat);
        grass_shader.uniform("u_view_matrix", m_camera.get_view());
        grass_shader.uniform("u_projection_matrix", m_proj_settings.projection_mat);
        grass_shader.uniform("u_time", (float)glfwGetTime());
        grass_shader.uniform("u_wind_strength", wind_strength);
        grass_shader.uniform("u_wind_direction", glm::normalize(wind_direction));
        grass_shader.uniform("u_bottom_color", bottom_color);
        grass_shader.uniform("u_top_color", top_color);
        
        grass_transform_buffer.bind_base(0);
        m_renderer.render_instanced(GL_TRIANGLES, grass_shader, grass, grass_transform_matrix.size());
        
        terrain_shader.uniform("u_model_matrix", terrain_transform_matrix);
        terrain_shader.uniform("u_normal_matrix", terrain_normal_matrix);
        m_renderer.render(GL_TRIANGLES, terrain_shader, terrain);

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
            } 
        ImGui::End();

        ImGui::Begin("Wind");
            ImGui::DragFloat3("direction", glm::value_ptr(wind_direction), 0.1f);
            ImGui::DragFloat("strength", &wind_strength, 0.01f, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        ImGui::End();

        ImGui::Begin("Grass");
            ImGui::ColorEdit3("top color", glm::value_ptr(top_color));
            ImGui::ColorEdit3("bottom color", glm::value_ptr(bottom_color));
        ImGui::End();

        // ImGui::Begin("Texture");
        //     texture_2d& texture = ao_map;
        //     const ImVec2 buffer_size_imvec2(texture.get_width(), texture.get_height());
        //     ImGui::Image((void*)(uintptr_t)texture.get_id(), buffer_size_imvec2, ImVec2(0, 1), ImVec2(1, 0));
        // ImGui::End();

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
