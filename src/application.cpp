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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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

constexpr float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void application::run() noexcept {
    shader gpass_shader(RESOURCE_DIR "shaders/ssao/gpass.vert", RESOURCE_DIR "shaders/ssao/gpass.frag");
    shader ssao_shader(RESOURCE_DIR "shaders/ssao/ssao.vert", RESOURCE_DIR "shaders/ssao/ssao.frag");
    shader blur_shader(RESOURCE_DIR "shaders/ssao/blur.vert", RESOURCE_DIR "shaders/ssao/blur.frag");
    shader lightpass_shader(RESOURCE_DIR "shaders/ssao/lightpass.vert", RESOURCE_DIR "shaders/ssao/lightpass.frag");

    framebuffer gpass_fbo;
    gpass_fbo.create();

    texture_2d position_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    position_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    position_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    position_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    position_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    texture_2d normal_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    normal_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    normal_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    normal_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    normal_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    texture_2d color_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    color_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    color_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    color_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    color_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    renderbuffer depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);

    assert(gpass_fbo.attach(GL_COLOR_ATTACHMENT0, 0, position_buffer));
    assert(gpass_fbo.attach(GL_COLOR_ATTACHMENT1, 0, normal_buffer));
    assert(gpass_fbo.attach(GL_COLOR_ATTACHMENT2, 0, color_buffer));
    assert(gpass_fbo.attach(GL_DEPTH_ATTACHMENT, depth_buffer));

    uint32_t attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    gpass_fbo.set_draw_buffer(3, attachments);


    framebuffer ssao_fbo;
    ssao_fbo.create();

    texture_2d ssao_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RED, GL_RED, GL_FLOAT);
    ssao_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ssao_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    assert(ssao_fbo.attach(GL_COLOR_ATTACHMENT0, 0, ssao_buffer));

    std::vector<glm::vec3> ssao_kernel(64);
    for(size_t i = 0; i < ssao_kernel.size(); ++i) {
        glm::vec3 sample = glm::normalize(glm::vec3(random(-1.0f, 1.0f), random(-1.0f, 1.0f), random(0.0f, 1.0f)));
        sample *= random(0.0f, 1.0f);

        float scale = static_cast<float>(i) / ssao_kernel.size();
        scale = lerp(0.1f, 1.0f, scale * scale);

        ssao_kernel[i] = sample * scale;
        ssao_shader.uniform("u_kernel[" + std::to_string(i) + "]", ssao_kernel[i]);
    }


    framebuffer ssaoblur_fbo;
    ssaoblur_fbo.create();

    texture_2d ssaoblur_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RED, GL_RED, GL_FLOAT);
    ssaoblur_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ssaoblur_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    assert(ssaoblur_fbo.attach(GL_COLOR_ATTACHMENT0, 0, ssaoblur_buffer));

    std::vector<glm::vec3> ssao_noise(16);
    for (size_t i = 0; i < ssao_noise.size(); ++i) {
        ssao_noise[i] = glm::vec3(random(-1.0f, 1.0f), random(-1.0f, 1.0f), 0.0f);
    }

    texture_2d ssao_noise_buffer(4, 4, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGB, GL_FLOAT, ssao_noise.data());
    ssao_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ssao_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ssao_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    ssao_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

    mesh plane(
        std::vector<mesh::vertex>{
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
            { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
        }, 
        { 0, 2, 1, 0, 3, 2 }
    );

    model cube(RESOURCE_DIR "models/cube/cube.obj", std::nullopt);

    model::texture_load_config config;
    config.mag_filter = GL_LINEAR;
    config.min_filter = GL_LINEAR;
    config.wrap_s = GL_CLAMP_TO_EDGE;
    config.wrap_t = GL_CLAMP_TO_EDGE;
    config.use_gamma = false;
    config.generate_mipmap = true;
    config.flip_on_load = true;
    model backpack(RESOURCE_DIR "models/backpack/backpack.obj", config);

    glm::vec3 light_position(2.0f), light_color(1.0f);
    float intensity = 1.0f;

    int32_t buffer_number = 0;
    bool use_ssao = true;

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

        gpass_fbo.bind();
        m_renderer.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gpass_shader.uniform("u_projection", m_proj_settings.projection_mat);
        gpass_shader.uniform("u_view", m_camera.get_view());
        gpass_shader.uniform("u_reverse_normals", true);
        gpass_shader.uniform("u_use_flat_color", true);
        gpass_shader.uniform("u_color", glm::vec3(1.0f));
        gpass_shader.uniform("u_model", glm::scale(glm::mat4(1.0f), glm::vec3(30.0f)));
        m_renderer.render(GL_TRIANGLES, gpass_shader, cube);
        gpass_shader.uniform("u_model", glm::translate(glm::mat4(1.0f), light_position) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)));
        gpass_shader.uniform("u_color", light_color);
        m_renderer.render(GL_TRIANGLES, gpass_shader, cube);
        gpass_shader.uniform("u_reverse_normals", false);
        gpass_shader.uniform("u_use_flat_color", false);
        gpass_shader.uniform("u_model", glm::mat4(1.0f));
        m_renderer.render(GL_TRIANGLES, gpass_shader, backpack);


        ssao_fbo.bind();
        m_renderer.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        m_renderer.clear(GL_COLOR_BUFFER_BIT);
        ssao_shader.uniform("u_screen_size", glm::vec2(m_proj_settings.width, m_proj_settings.height));
        ssao_shader.uniform("u_gbuffer.position", 0);
        position_buffer.bind(0);
        ssao_shader.uniform("u_gbuffer.normal", 1);
        normal_buffer.bind(1);
        ssao_shader.uniform("u_noise_buffer", 2);
        ssao_noise_buffer.bind(2);
        ssao_shader.uniform("u_noise_scale", 4.0f);
        ssao_shader.uniform("u_projection", m_proj_settings.projection_mat);
        m_renderer.render(GL_TRIANGLES, ssao_shader, plane);


        ssaoblur_fbo.bind();
        m_renderer.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        m_renderer.clear(GL_COLOR_BUFFER_BIT);
        blur_shader.uniform("u_ssao_buffer", 0);
        ssao_buffer.bind(0);
        m_renderer.render(GL_TRIANGLES, blur_shader, plane);


        framebuffer::bind_default();
        m_renderer.set_clear_color(m_clear_color);
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightpass_shader.uniform("u_ssaoblur_buffer", 0);
        ssaoblur_buffer.bind(0);
        lightpass_shader.uniform("u_gbuffer.position", 1);
        position_buffer.bind(1);
        lightpass_shader.uniform("u_gbuffer.normal", 2);
        normal_buffer.bind(2);
        lightpass_shader.uniform("u_gbuffer.albedo", 3);
        color_buffer.bind(3);
        lightpass_shader.uniform("u_light.position_viewspace", glm::vec3(m_camera.get_view() * glm::vec4(light_position, 1.0f)));
        lightpass_shader.uniform("u_light.color", light_color);
        lightpass_shader.uniform("u_light.intensity", intensity);
        lightpass_shader.uniform("u_light.constant", 1.0f);
        lightpass_shader.uniform("u_light.linear", 0.09f);
        lightpass_shader.uniform("u_light.quadratic", 0.032f);
        lightpass_shader.uniform("u_material.shininess", 64.0f);
        m_renderer.render(GL_TRIANGLES, lightpass_shader, plane);

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

            if(ImGui::Checkbox("use SSAO", &use_ssao)) {
                lightpass_shader.uniform("u_use_ssao", use_ssao);
            }

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Light");
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            if (ImGui::Button("X")) {
                light_position.x = 0.0f;
            }
            ImGui::PushItemWidth(70);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
            ImGui::SameLine(); ImGui::DragFloat("##X", &light_position.x, 0.1f);
            if ((ImGui::SameLine(), ImGui::Button("Y"))) {
                light_position.y = 0.0f;
            }
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.6f, 1.0f));
            ImGui::SameLine(); ImGui::DragFloat("##Y", &light_position.y, 0.1f);
            if ((ImGui::SameLine(), ImGui::Button("Z"))) {
                light_position.z = 0.0f;
            }
            ImGui::SameLine(); ImGui::DragFloat("##Z", &light_position.z, 0.1f);
            ImGui::PopStyleColor(3);
            ImGui::PopItemWidth();
            ImGui::ColorEdit3("color", glm::value_ptr(light_color));
            ImGui::DragFloat("intensity", &intensity, 0.1f);
        ImGui::End();

        // ImGui::Begin("Backpack");
        //     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        //     if (ImGui::Button("X")) {
        //         backpack_position.x = 0.0f;
        //     }
        //     ImGui::PushItemWidth(70);
        //     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
        //     ImGui::SameLine(); ImGui::DragFloat("##X", &backpack_position.x, 0.1f);
        //     if ((ImGui::SameLine(), ImGui::Button("Y"))) {
        //         backpack_position.y = 0.0f;
        //     }
        //     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.6f, 1.0f));
        //     ImGui::SameLine(); ImGui::DragFloat("##Y", &backpack_position.y, 0.1f);
        //     if ((ImGui::SameLine(), ImGui::Button("Z"))) {
        //         backpack_position.z = 0.0f;
        //     }
        //     ImGui::SameLine(); ImGui::DragFloat("##Z", &backpack_position.z, 0.1f);
        //     ImGui::PopStyleColor(3);
        //     ImGui::PopItemWidth();
        // ImGui::End();

        // ImGui::Begin("Texture");
        //     const uint32_t buffers[] = { position_buffer.get_id(), color_buffer.get_id(), normal_buffer.get_id(), ssao_buffer.get_id(), ssaoblur_buffer.get_id() };
            
        //     ImGui::SliderInt("buffer number", &buffer_number, 0, sizeof(buffers) / sizeof(buffers[0]) - 1);
        //     ImGui::Image(
        //         (void*)(intptr_t)buffers[buffer_number], 
        //         ImVec2(position_buffer.get_width(), position_buffer.get_height()), 
        //         ImVec2(0, 1), 
        //         ImVec2(1, 0)
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

void application::glfw_deinitializer::operator()(bool *is_glfw_initialized) noexcept {
    glfwTerminate();
    if (is_glfw_initialized != nullptr) {
        *is_glfw_initialized = false;
    }
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
