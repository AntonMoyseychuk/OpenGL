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

    // m_renderer.enable(GL_STENCIL_TEST);
    //
    // m_renderer.enable(GL_BLEND);
    // m_renderer.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _imgui_init("#version 430 core");
}

void application::run() noexcept {    
    float cube_vertices[] = {
		// back face
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
		1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		// front face
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
		1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		// left face
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		// right face
		1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
		1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
		// bottom face
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
		1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		// top face
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
		1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
	};

    float plane_vertices[] = {
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    };

    mesh plane(std::vector<mesh::vertex>((mesh::vertex*)plane_vertices, (mesh::vertex*)plane_vertices + sizeof(plane_vertices) / sizeof(mesh::vertex)),
        {0, 2, 1, 0, 3, 2});

    mesh cube(std::vector<mesh::vertex>((mesh::vertex*)cube_vertices, (mesh::vertex*)cube_vertices + sizeof(cube_vertices) / sizeof(mesh::vertex)), {});
    // texture_2d texture(RESOURCE_DIR "textures/container.png", GL_TEXTURE_2D, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, true);
    // texture.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    // texture.set_tex_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    // texture.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // texture.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // texture.generate_mipmap();
    // texture_2d spec_texture(RESOURCE_DIR "textures/container_specular.png", GL_TEXTURE_2D, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, true);
    // spec_texture.set_tex_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    // spec_texture.set_tex_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    // spec_texture.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // spec_texture.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // spec_texture.generate_mipmap();
    model::texture_config config;
    config.target = GL_TEXTURE_2D;
    config.level = 0;
    config.internal_format = GL_RGB;
    config.format = GL_RGB;
    config.type = GL_UNSIGNED_BYTE;
    config.mag_filter = GL_LINEAR;
    config.min_filter = GL_LINEAR;
    config.wrap_s = GL_REPEAT;
    config.wrap_t = GL_REPEAT;
    config.generate_mipmap = true;
    config.flip_on_load = true;
    model backpack(RESOURCE_DIR "models/backpack/backpack.obj", config);

    framebuffer g_framebuffer;
    g_framebuffer.create();
    
    texture_2d position_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    position_buffer.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    position_buffer.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g_framebuffer.attach(GL_COLOR_ATTACHMENT0, 0, position_buffer);

    texture_2d normal_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    normal_buffer.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    normal_buffer.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g_framebuffer.attach(GL_COLOR_ATTACHMENT1, 0, normal_buffer);

    texture_2d albedo_spec_buffer(m_proj_settings.width, m_proj_settings.height, GL_TEXTURE_2D, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    albedo_spec_buffer.set_tex_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    albedo_spec_buffer.set_tex_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    g_framebuffer.attach(GL_COLOR_ATTACHMENT2, 0, albedo_spec_buffer);

    uint32_t attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    renderbuffer depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);
    ASSERT(g_framebuffer.attach(GL_DEPTH_ATTACHMENT, depth_buffer), "G Buffer", "invalid G Buffer");

    shader gpass_shader(RESOURCE_DIR "shaders/deffered_rendering/gpass.vert", RESOURCE_DIR "shaders/deffered_rendering/gpass.frag");
    shader colorpass_shader(RESOURCE_DIR "shaders/deffered_rendering/colorpass.vert", RESOURCE_DIR "shaders/deffered_rendering/colorpass.frag");
    shader light_source_shader(RESOURCE_DIR "shaders/deffered_rendering/light_source.vert", RESOURCE_DIR "shaders/deffered_rendering/light_source.frag");

    std::vector<glm::mat4> instance_model_matrices {
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, -0.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, -0.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, -0.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, -0.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, -0.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, -0.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, -0.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, -0.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, -0.5,  2.0)), glm::vec3(0.35f)),
        
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 1.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 1.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 1.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 1.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 1.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 1.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 1.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 1.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 1.5,  2.0)), glm::vec3(0.35f)),

        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 3.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 3.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 3.5, -2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 3.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 3.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 3.5,  0.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0, 3.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 0.0, 3.5,  2.0)), glm::vec3(0.35f)),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3( 2.0, 3.5,  2.0)), glm::vec3(0.35f)),
    };
    buffer instance_models_shader_buffer(GL_SHADER_STORAGE_BUFFER, instance_model_matrices.size() * sizeof(glm::mat4), sizeof(glm::mat4), 
        GL_STATIC_DRAW, instance_model_matrices.data());
    OGL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, instance_models_shader_buffer.id));
    
    constexpr size_t NR_LIGHTS = 32;
    std::vector<glm::vec3> light_positions;
    std::vector<glm::vec3> light_colors;
    srand(time(0));
    for (size_t i = 0; i < NR_LIGHTS; i++) {
        light_positions.push_back(glm::vec3(
            ((rand() % 100) / 100.0f) * 6.0f - 3.0f,
            ((rand() % 100) / 100.0f) * 6.0f - 2.0f,
            ((rand() % 100) / 100.0f) * 6.0f - 3.0f
        ));

        light_colors.push_back(glm::vec3(
            // ((rand() % 100) / 200.0f) + 0.5f,
            // ((rand() % 100) / 200.0f) + 0.5f,
            // ((rand() % 100) / 200.0f) + 0.5f
            rand() % 2,
            rand() % 2,
            rand() % 2
        ));
    }

    
    ImGuiIO& io = ImGui::GetIO();
    while (!glfwWindowShouldClose(m_window) && glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
        glfwPollEvents();

        m_camera.update_dt(io.DeltaTime);
        if (!m_camera.is_fixed) {
            if (glfwGetKey(m_window, GLFW_KEY_SPACE)) {
                m_camera.move(m_camera.get_up() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL)) {
                m_camera.move(-m_camera.get_up() * io.DeltaTime);
            }
            if (glfwGetKey(m_window, GLFW_KEY_W)) {
                m_camera.move(m_camera.get_forward() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_S)) {
                m_camera.move(-m_camera.get_forward() * io.DeltaTime);
            }
            if (glfwGetKey(m_window, GLFW_KEY_D)) {
                m_camera.move(m_camera.get_right() * io.DeltaTime);
            } else if (glfwGetKey(m_window, GLFW_KEY_A)) {
                m_camera.move(-m_camera.get_right() * io.DeltaTime);
            }
        }


        g_framebuffer.bind();
        m_renderer.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gpass_shader.uniform("u_projection", m_proj_settings.projection_mat);
        gpass_shader.uniform("u_view", m_camera.get_view());
        gpass_shader.uniform("u_material.albedo_texture", 0);
        gpass_shader.uniform("u_material.specular_texture", 1);
        m_renderer.render_instanced(GL_TRIANGLES, gpass_shader, backpack, instance_model_matrices.size());

        g_framebuffer.unbind();
        m_renderer.set_clear_color(m_clear_color.r, m_clear_color.g, m_clear_color.b, 1.0f);
        m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        colorpass_shader.uniform("u_gbuffer.position_buffer", 0);
        colorpass_shader.uniform("u_gbuffer.normal_buffer", 1);
        colorpass_shader.uniform("u_gbuffer.albedo_spec_buffer", 2);
        position_buffer.bind(0);
        normal_buffer.bind(1);
        albedo_spec_buffer.bind(2);

        colorpass_shader.uniform("u_material.shininess", 64.0f);
        for (size_t i = 0; i < light_positions.size(); i++) {
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].position", light_positions[i]);
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].color", light_colors[i]);
            
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].intensity", 1.2f);

            const float linear = 0.7f;
            const float quadratic = 1.8f;
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].constant", 1.0f);
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].linear", linear);
            colorpass_shader.uniform("u_lights[" + std::to_string(i) + "].quadratic", quadratic);
        }
        colorpass_shader.uniform("u_camera_position", m_camera.position);
        
        m_renderer.render(GL_TRIANGLES, colorpass_shader, plane);

        OGL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, g_framebuffer.get_id()));
        OGL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
        OGL_CALL(glBlitFramebuffer(0, 0, m_proj_settings.width, m_proj_settings.height, 0, 0, 
            m_proj_settings.width, m_proj_settings.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST));
        OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        light_source_shader.uniform("u_projection", m_proj_settings.projection_mat);
        light_source_shader.uniform("u_view", m_camera.get_view());
        for (size_t i = 0; i < NR_LIGHTS; ++i) {
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), light_positions[i]), glm::vec3(0.1f)));
            light_source_shader.uniform("u_color", glm::vec4(light_colors[i], 1.0f));
            m_renderer.render(GL_TRIANGLES, light_source_shader, cube);
        }


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

        // ImGui::Begin("GBuffer");
        //     ImGui::Image((void*)(intptr_t)normal_buffer.get_id(), ImVec2(m_proj_settings.width, m_proj_settings.height), ImVec2(0, 1), ImVec2(1, 0));
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
