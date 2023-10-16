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

    m_renderer.enable(GL_CLIP_DISTANCE0);
}

void application::run() noexcept {
    ImGuiIO& io = ImGui::GetIO();

    shader gpass_shader(RESOURCE_DIR "shaders/ssao/gpass.vert", RESOURCE_DIR "shaders/ssao/gpass.frag");
    shader ssao_shader(RESOURCE_DIR "shaders/ssao/ssao.vert", RESOURCE_DIR "shaders/ssao/ssao.frag");
    shader lightpass_shader(RESOURCE_DIR "shaders/ssao/lightpass.vert", RESOURCE_DIR "shaders/ssao/lightpass.frag");
    shader blur_shader(RESOURCE_DIR "shaders/ssao/blur.vert", RESOURCE_DIR "shaders/ssao/blur.frag");

    framebuffer gbuffer;
    gbuffer.create();

    texture_2d position_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    position_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    position_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    position_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    position_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gbuffer.attach(GL_COLOR_ATTACHMENT0, 0, position_buffer));

    texture_2d normal_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    normal_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    normal_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    normal_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    normal_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gbuffer.attach(GL_COLOR_ATTACHMENT1, 0, normal_buffer));
    
    renderbuffer depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);
    assert(gbuffer.attach(GL_DEPTH_ATTACHMENT, depth_buffer));

    uint32_t draw_buffer_attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    gbuffer.set_draw_buffer(2, draw_buffer_attachments);


    std::vector<glm::vec3> ssao_kernel(64);
    ssao_shader.uniform("u_samples_count", ssao_kernel.size());
    for (size_t i = 0; i < ssao_kernel.size(); ++i) {
        ssao_kernel[i] = glm::normalize(glm::vec3(random(-1.0f, 1.0f), random(-1.0f, 1.0f), random(0.0f, 1.0f)));
        ssao_kernel[i] *= random(0.0f, 1.0f);
        ssao_shader.uniform("u_ssao_samples[" + std::to_string(i) + "]", ssao_kernel[i]);
    }


    std::vector<glm::vec3> ssao_noise(16);
    for (auto& noise : ssao_noise) {
        noise = glm::vec3(random(-1.0f, 1.0f), random(-1.0f, 1.0f), 0.0f);
    }
    texture_2d noise_texture(4, 4, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT, ssao_noise.data());
    noise_texture.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    noise_texture.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    noise_texture.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    noise_texture.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);


    framebuffer ssao_framebuffer;
    ssao_framebuffer.create();

    texture_2d ssao_color_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RED, GL_RED, GL_FLOAT);
    ssao_color_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ssao_color_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(ssao_framebuffer.attach(GL_COLOR_ATTACHMENT0, 0, ssao_color_buffer));


    framebuffer ssao_blur_framebuffer;
    ssao_blur_framebuffer.create();

    texture_2d ssao_blur_color_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RED, GL_RED, GL_FLOAT);
    ssao_blur_color_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ssao_blur_color_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(ssao_blur_framebuffer.attach(GL_COLOR_ATTACHMENT0, 0, ssao_blur_color_buffer));


    model backpack(RESOURCE_DIR "models/backpack/backpack.obj", std::nullopt);

    mesh plane(
        std::vector<mesh::vertex>{
            { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
            { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
            { { 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
        }, 
        { 0, 2, 1, 0, 3, 2 }
    );

    float ssao_noise_radius = 0.5f;
    float ssao_bias = 0.025f;

    glm::vec3 light_position(2.0f), light_color(1.0f);
    float light_intencity = 1.0f;

    uint32_t debug_gbuffer_ids[] = { position_buffer.get_id(), normal_buffer.get_id() };
    int32_t debug_buffer_number = 0;

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

        gbuffer.bind();
        m_renderer.set_clear_color(glm::vec4(0.0f));
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gpass_shader.uniform("u_projection_matrix", m_proj_settings.projection_mat);
        gpass_shader.uniform("u_view_matrix", m_camera.get_view());
        gpass_shader.uniform("u_model_matrix", glm::mat4(1.0f));
        gpass_shader.uniform("u_normal_matrix", glm::mat4(1.0f));
        m_renderer.render(GL_TRIANGLES, gpass_shader, backpack);

        ssao_framebuffer.bind();
        m_renderer.set_clear_color(glm::vec4(0.0f));
        m_renderer.clear(GL_COLOR_BUFFER_BIT);
        ssao_shader.uniform("u_gbuffer.position", position_buffer, 0);
        ssao_shader.uniform("u_gbuffer.normal", normal_buffer, 1);
        ssao_shader.uniform("u_ssao_noise", noise_texture, 2);
        ssao_shader.uniform("u_screen_size", glm::vec2(m_proj_settings.width, m_proj_settings.height));
        ssao_shader.uniform("u_noise_radius", ssao_noise_radius);
        ssao_shader.uniform("u_ssao_bias", ssao_bias);
        ssao_shader.uniform("u_projection_matrix", m_proj_settings.projection_mat);
        m_renderer.render(GL_TRIANGLES, ssao_shader, plane);


        ssao_blur_framebuffer.bind();
        m_renderer.set_clear_color(glm::vec4(0.0f));
        m_renderer.clear(GL_COLOR_BUFFER_BIT);
        blur_shader.uniform("u_ssao_buffer", ssao_color_buffer, 0);
        m_renderer.render(GL_TRIANGLES, blur_shader, plane);


        framebuffer::bind_default();
        m_renderer.set_clear_color(m_clear_color);
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightpass_shader.uniform("u_gbuffer.position", position_buffer, 0);
        lightpass_shader.uniform("u_gbuffer.normal", normal_buffer, 1);
        lightpass_shader.uniform("u_ssaoblur_buffer", ssao_blur_color_buffer, 2);
        lightpass_shader.uniform("u_light.position_viewspace", glm::vec3(m_camera.get_view() * glm::vec4(light_position, 1.0f)));
        lightpass_shader.uniform("u_light.color", light_color);
        lightpass_shader.uniform("u_light.intensity", light_intencity);
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

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("SSAO");
            ImGui::DragFloat("radius", &ssao_noise_radius, 0.01f, 0.0f, std::numeric_limits<float>::max());
            ImGui::DragFloat("bias", &ssao_bias, 0.01f, 0.0f, std::numeric_limits<float>::max());
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
            ImGui::DragFloat("intensity", &light_intencity, 0.1f, 0.0f, 1.0f);
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

        ImGui::Begin("G Buffer");
            ImGui::SliderInt("buffer number", &debug_buffer_number, 0, 3);
            {
                const ImVec2 buffer_size_imvec2(position_buffer.get_width(), position_buffer.get_height());
                switch (debug_buffer_number) {
                    case 0: ImGui::Image((void*)(uintptr_t)position_buffer.get_id(), buffer_size_imvec2, ImVec2(0, 1), ImVec2(1, 0)); break;
                    case 1: ImGui::Image((void*)(uintptr_t)normal_buffer.get_id(), buffer_size_imvec2, ImVec2(0, 1), ImVec2(1, 0)); break;
                    case 2: ImGui::Image((void*)(uintptr_t)ssao_color_buffer.get_id(), buffer_size_imvec2, ImVec2(0, 1), ImVec2(1, 0)); break;
                    case 3: ImGui::Image((void*)(uintptr_t)ssao_blur_color_buffer.get_id(), buffer_size_imvec2, ImVec2(0, 1), ImVec2(1, 0)); break;
                }
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
