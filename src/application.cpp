#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#include "debug.hpp"

#include "shader.hpp"
#include "texture.hpp"
#include "model.hpp"

#include "light/spot_light.hpp"
#include "light/point_light.hpp"
#include "light/directional_light.hpp"

#include <algorithm>
#include <map>

std::unique_ptr<bool, application::glfw_deinitializer> application::is_glfw_initialized(new bool(glfwInit() == GLFW_TRUE));

application &application::get(const std::string_view &title, uint32_t width, uint32_t height) noexcept {
    static application app(title, width, height);
    return app;
}

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title), m_camera(glm::vec3(0.0f, 1.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 12.0f, 10.0f)
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

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
    ASSERT(m_window != nullptr, "GLFW error", (_destroy(), glfwGetError(&glfw_error_msg), glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error"));
    glfwSetWindowUserPointer(m_window, this);

    glfwMakeContextCurrent(m_window);
    ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "GLAD error", "failed to initialize GLAD");
    glfwSwapInterval(1);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    glfwSetCursorPosCallback(m_window, &application::_mouse_callback);
    glfwSetScrollCallback(m_window, &application::_mouse_wheel_scroll_callback);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 100.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);


    OGL_CALL(glEnable(GL_DEPTH_TEST));
    OGL_CALL(glEnable(GL_STENCIL_TEST));

    OGL_CALL(glEnable(GL_BLEND));
    OGL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)); 

    _imgui_init("#version 430");
}

void application::run() noexcept {
    // config.type = texture::type::EMISSION;
    // texture emission_map(RESOURCE_DIR "textures/matrix.jpg", config);
    // config.type = texture::type::SPECULAR;
    // texture specular_map(RESOURCE_DIR "textures/container_specular.png", config);
    //
    
    texture::config config(GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true, texture::type::NONE);
    texture diffuse_map(RESOURCE_DIR "textures/blending_transparent_window.png", config);
    
    std::vector<glm::vec3> cube_positions = {
        glm::vec3( 0.0f,  0.0f,  0.0f), 
        glm::vec3( 2.0f,  5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f),  
        glm::vec3(-3.8f, -2.0f, -12.3f),  
        glm::vec3( 2.4f, -0.4f, -3.5f),  
        glm::vec3(-1.7f,  3.0f, -7.5f),  
        glm::vec3( 1.3f, -2.0f, -2.5f),  
        glm::vec3( 1.5f,  2.0f, -2.5f), 
        glm::vec3( 1.5f,  0.2f, -1.5f), 
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    struct transform {
        glm::vec3 scale = glm::vec3(1.0f);
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
    } backpack_transform, sponza_transform = { glm::vec3(0.02f), glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(0.0f) };

    model cube(RESOURCE_DIR "models/cube/cube.obj");
    model backpack(RESOURCE_DIR "models/backpack/backpack.obj");
    model sponza(RESOURCE_DIR "models/Sponza/sponza.obj");

    std::vector<point_light> point_lights = {
        point_light(glm::vec3(1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3(-3.0f,  3.0f, -3.0f), 0.09f, 0.032f),
        point_light(glm::vec3(1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3( 3.0f), 0.09f, 0.032f),
        point_light(glm::vec3(1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3(-3.0f, -3.0f,  3.0f), 0.09f, 0.032f),
        point_light(glm::vec3(1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3( 3.0f, -3.0f, -3.0f), 0.09f, 0.032f),
    };
    std::vector<spot_light> spot_lights = {
        spot_light(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3( 0.0f,  5.0f, -2.0f), glm::vec3( 0.0f, -1.0f, 0.0f), 15.0f),
        spot_light(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3( 0.0f, -5.0f, -2.0f), glm::vec3( 0.0f,  1.0f, 0.0f), 15.0f),
        spot_light(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3(-5.0f,  0.0f, -2.0f), glm::vec3( 1.0f,  0.0f, 0.0f), 15.0f),
        spot_light(glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.05f), glm::vec3(0.5f), glm::vec3(1.0f), glm::vec3( 5.0f,  0.0f, -2.0f), glm::vec3(-1.0f,  0.0f, 0.0f), 15.0f),
    };
    directional_light directional_light(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.05f), glm::vec3(0.8f), glm::vec3(1.0f), glm::vec3(-1.0f, -1.0f, -1.0f));

    shader light_source_shader(RESOURCE_DIR "shaders/light_source.vert", RESOURCE_DIR "shaders/light_source.frag");

    shader scene_shader(RESOURCE_DIR "shaders/light.vert", RESOURCE_DIR "shaders/light.frag");
    scene_shader.uniform("u_point_lights_count", (uint32_t)point_lights.size());
    scene_shader.uniform("u_spot_lights_count", (uint32_t)spot_lights.size());

    shader border_shader(RESOURCE_DIR "shaders/light_source.vert", RESOURCE_DIR "shaders/single_color.frag");
    border_shader.uniform("u_border_color", glm::vec3(1.0f));

    shader texture_shader(RESOURCE_DIR "shaders/post_process.vert", RESOURCE_DIR "shaders/post_process.frag");

    uint32_t fbo;
    OGL_CALL(glGenFramebuffers(1, &fbo));
    OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    uint32_t color_buffer;
    OGL_CALL(glGenTextures(1, &color_buffer));
    OGL_CALL(glBindTexture(GL_TEXTURE_2D, color_buffer));
    OGL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_proj_settings.width, m_proj_settings.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
    OGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    OGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    OGL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

    OGL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0));

    uint32_t depth_stencil_buffer;
    OGL_CALL(glGenRenderbuffers(1, &depth_stencil_buffer));
    OGL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer));
    OGL_CALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_proj_settings.width, m_proj_settings.height));
    OGL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    OGL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer));
    
    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "framebuffer error", "framebuffer is incomplit");
    OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    mesh plane({ 
        mesh::vertex { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f) },
        mesh::vertex { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f) },
        mesh::vertex { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f) },
        mesh::vertex { glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f) },
    }, {
        0, 2, 1,
        0, 3, 2
    }, {});

    ImGuiIO& io = ImGui::GetIO();
    float material_shininess = 64.0f;
    std::multimap<float, glm::vec3> distances_to_transparent_cubes;

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

        OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        OGL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

        OGL_CALL(glEnable(GL_CULL_FACE));

        m_camera.rotate(180.0f, glm::vec2(0.0f, 1.0f));

        light_source_shader.uniform("u_view", m_camera.get_view());
        light_source_shader.uniform("u_projection", m_proj_settings.projection_mat);
        for (size_t i = 0; i < point_lights.size(); ++i) {
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), point_lights[i].position), glm::vec3(0.2f)));
            light_source_shader.uniform("u_light_settings.color", point_lights[i].color);
            cube.draw(light_source_shader);
        }
        for (size_t i = 0; i < spot_lights.size(); ++i) {
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), spot_lights[i].position), glm::vec3(0.2f)));
            light_source_shader.uniform("u_light_settings.color", spot_lights[i].color);
            cube.draw(light_source_shader);
        }
        
        scene_shader.uniform("u_view", m_camera.get_view());
        scene_shader.uniform("u_projection", m_proj_settings.projection_mat);
        scene_shader.uniform("u_view_position", m_camera.position);
        const double t = glfwGetTime();
        scene_shader.uniform("u_time", t - size_t(t));
        scene_shader.uniform("u_material.shininess", material_shininess);

        for (size_t i = 0; i < point_lights.size(); ++i) {
            const std::string str_i = std::to_string(i);
            
            scene_shader.uniform(("u_point_lights[" + str_i + "].position").c_str(), point_lights[i].position);

            scene_shader.uniform(("u_point_lights[" + str_i + "].linear"   ).c_str(), point_lights[i].linear);
            scene_shader.uniform(("u_point_lights[" + str_i + "].quadratic").c_str(), point_lights[i].quadratic);
            
            scene_shader.uniform(("u_point_lights[" + str_i + "].ambient" ).c_str(), point_lights[i].color * point_lights[i].ambient);
            scene_shader.uniform(("u_point_lights[" + str_i + "].diffuse" ).c_str(), point_lights[i].color * point_lights[i].diffuse);
            scene_shader.uniform(("u_point_lights[" + str_i + "].specular").c_str(), point_lights[i].color * point_lights[i].specular);
        }
        for (size_t i = 0; i < spot_lights.size(); ++i) {
            const std::string str_i = std::to_string(i);
            
            scene_shader.uniform(("u_spot_lights[" + str_i + "].position" ).c_str(), spot_lights[i].position);
            scene_shader.uniform(("u_spot_lights[" + str_i + "].direction").c_str(), glm::normalize(spot_lights[i].direction));
            scene_shader.uniform(("u_spot_lights[" + str_i + "].cutoff"   ).c_str(), glm::cos(glm::radians(spot_lights[i].cutoff)));

            scene_shader.uniform(("u_spot_lights[" + str_i + "].ambient" ).c_str(),  spot_lights[i].color * spot_lights[i].ambient);
            scene_shader.uniform(("u_spot_lights[" + str_i + "].diffuse" ).c_str(),  spot_lights[i].color * spot_lights[i].diffuse);
            scene_shader.uniform(("u_spot_lights[" + str_i + "].specular").c_str(), spot_lights[i].color * spot_lights[i].specular);
        }

        scene_shader.uniform("u_dir_light.direction", glm::normalize(directional_light.direction));
        scene_shader.uniform("u_dir_light.ambient", directional_light.color  * directional_light.ambient);
        scene_shader.uniform("u_dir_light.diffuse", directional_light.color  * directional_light.diffuse);
        scene_shader.uniform("u_dir_light.specular", directional_light.color * directional_light.specular);
        
        glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), sponza_transform.position) 
            * glm::mat4_cast(glm::quat(sponza_transform.rotation * (glm::pi<float>() / 180.0f)))
            * glm::scale(glm::mat4(1.0f), sponza_transform.scale);
        scene_shader.uniform("u_model", model_matrix);
        scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
        scene_shader.uniform("u_flip_texture", true);
        sponza.draw(scene_shader);

        model_matrix = glm::translate(glm::mat4(1.0f), backpack_transform.position) 
            * glm::mat4_cast(glm::quat(backpack_transform.rotation * (glm::pi<float>() / 180.0f)))
            * glm::scale(glm::mat4(1.0f), backpack_transform.scale);
        scene_shader.uniform("u_model", model_matrix);
        scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
        scene_shader.uniform("u_flip_texture", false);
        backpack.draw(scene_shader);

        OGL_CALL(glDisable(GL_CULL_FACE));

        distances_to_transparent_cubes.clear();
        for (size_t i = 0; i < cube_positions.size(); ++i) {
            distances_to_transparent_cubes.insert(std::make_pair(glm::length2(m_camera.position - cube_positions[i]), cube_positions[i]));
        }

        for (auto& it = distances_to_transparent_cubes.rbegin(); it != distances_to_transparent_cubes.rend(); ++it) {
            model_matrix = glm::scale(glm::translate(glm::mat4(1.0f), it->second), glm::vec3(0.7f));
            scene_shader.uniform("u_model", model_matrix);
            scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
            diffuse_map.activate_unit(0);
            scene_shader.uniform("u_material.diffuse0", 0);
            cube.draw(scene_shader);
        }


        OGL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        OGL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        OGL_CALL(glEnable(GL_CULL_FACE));

        m_camera.rotate(180.0f, glm::vec2(0.0f, 1.0f));

        light_source_shader.uniform("u_view", m_camera.get_view());
        light_source_shader.uniform("u_projection", m_proj_settings.projection_mat);
        for (size_t i = 0; i < point_lights.size(); ++i) {
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), point_lights[i].position), glm::vec3(0.2f)));
            light_source_shader.uniform("u_light_settings.color", point_lights[i].color);
            cube.draw(light_source_shader);
        }
        for (size_t i = 0; i < spot_lights.size(); ++i) {
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), spot_lights[i].position), glm::vec3(0.2f)));
            light_source_shader.uniform("u_light_settings.color", spot_lights[i].color);
            cube.draw(light_source_shader);
        }
        
        scene_shader.uniform("u_view", m_camera.get_view());
        
        model_matrix = glm::translate(glm::mat4(1.0f), sponza_transform.position) 
            * glm::mat4_cast(glm::quat(sponza_transform.rotation * (glm::pi<float>() / 180.0f)))
            * glm::scale(glm::mat4(1.0f), sponza_transform.scale);
        scene_shader.uniform("u_model", model_matrix);
        scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
        scene_shader.uniform("u_flip_texture", true);
        sponza.draw(scene_shader);

        model_matrix = glm::translate(glm::mat4(1.0f), backpack_transform.position) 
            * glm::mat4_cast(glm::quat(backpack_transform.rotation * (glm::pi<float>() / 180.0f)))
            * glm::scale(glm::mat4(1.0f), backpack_transform.scale);
        scene_shader.uniform("u_model", model_matrix);
        scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
        scene_shader.uniform("u_flip_texture", false);
        backpack.draw(scene_shader);

        OGL_CALL(glDisable(GL_CULL_FACE));

        distances_to_transparent_cubes.clear();
        for (size_t i = 0; i < cube_positions.size(); ++i) {
            distances_to_transparent_cubes.insert(std::make_pair(glm::length2(m_camera.position - cube_positions[i]), cube_positions[i]));
        }

        for (auto& it = distances_to_transparent_cubes.rbegin(); it != distances_to_transparent_cubes.rend(); ++it) {
            model_matrix = glm::scale(glm::translate(glm::mat4(1.0f), it->second), glm::vec3(0.7f));
            scene_shader.uniform("u_model", model_matrix);
            scene_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model_matrix)));
            diffuse_map.activate_unit(0);
            scene_shader.uniform("u_material.diffuse0", 0);
            cube.draw(scene_shader);
        }

        OGL_CALL(glActiveTexture(GL_TEXTURE0));
        OGL_CALL(glBindTexture(GL_TEXTURE_2D, color_buffer));
        OGL_CALL(glViewport(m_proj_settings.width / 2 - 250, m_proj_settings.height - 150, 500, 150));
        texture_shader.uniform("u_texture", 0);
        OGL_CALL(glDisable(GL_DEPTH_TEST));
        plane.draw(texture_shader);
        OGL_CALL(glEnable(GL_DEPTH_TEST));

        OGL_CALL(glViewport(0, 0, m_proj_settings.width, m_proj_settings.height));

    #pragma region ImGui
        _imgui_frame_begin();

        ImGui::Begin("Information");
            ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION)); ImGui::NewLine();
                
            if (ImGui::Checkbox("wireframe mode", &m_wireframed)) {
                OGL_CALL(glPolygonMode(GL_FRONT_AND_BACK, (m_wireframed ? GL_LINE : GL_FILL)));
            }
                
            if (ImGui::NewLine(), ImGui::ColorEdit3("background color", glm::value_ptr(m_clear_color))) {
                OGL_CALL(glClearColor(m_clear_color.r, m_clear_color.g, m_clear_color.b, 1.0f));
            }

            ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
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

            if (ImGui::Checkbox("fixed (Press \'1\')", &m_camera.is_fixed)) {
                glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
            } else if (glfwGetKey(m_window, GLFW_KEY_1)) {
                m_camera.is_fixed = !m_camera.is_fixed;

                glfwSetInputMode(m_window, GLFW_CURSOR, (m_camera.is_fixed ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        ImGui::End();

        ImGui::Begin("Light");
            ImGui::TextColored({1.0f, 0.0f, 0.0f, 1.0f}, "directional-light"); 
            ImGui::DragFloat3("direction##dl", glm::value_ptr(directional_light.direction), 0.1f); 
            ImGui::ColorEdit3("color##dl", glm::value_ptr(directional_light.color));

            for(size_t i = 0; i < point_lights.size(); ++i) {
                ImGui::NewLine(); ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "point-light-%s", std::to_string(i).c_str()); 
                ImGui::DragFloat3(("position##pl" + std::to_string(i)).c_str(), glm::value_ptr(point_lights[i].position), 0.1f);
                ImGui::ColorEdit3(("color##pl" + std::to_string(i)).c_str() , glm::value_ptr(point_lights[i].color));
                ImGui::InputFloat(("linear##pl" + std::to_string(i)).c_str(), &point_lights[i].linear);
                ImGui::InputFloat(("quadratic##pl" + std::to_string(i)).c_str(), &point_lights[i].quadratic);
            }

            for(size_t i = 0; i < spot_lights.size(); ++i) {
                ImGui::NewLine(); ImGui::TextColored({0.0f, 0.0f, 1.0f, 1.0f}, "spot-light-%s", std::to_string(i).c_str()); 
                ImGui::DragFloat3(("position##sl" + std::to_string(i)).c_str(), glm::value_ptr(spot_lights[i].position), 0.1f);
                ImGui::DragFloat3(("direction##sl" + std::to_string(i)).c_str(), glm::value_ptr(spot_lights[i].direction), 0.1f);
                ImGui::ColorEdit3(("color##sl" + std::to_string(i)).c_str(), glm::value_ptr(spot_lights[i].color));
                ImGui::SliderFloat(("cutoff##sl" + std::to_string(i)).c_str(), &spot_lights[i].cutoff, 1.0f, 179.0f);
            }
        ImGui::End();

        ImGui::Begin("Backpack");
            ImGui::DragFloat3("position##backpack", glm::value_ptr(backpack_transform.position), 0.1f);
            ImGui::DragFloat3("rotation##backpack", glm::value_ptr(backpack_transform.rotation));
            ImGui::SliderFloat3("scale##backpack", glm::value_ptr(backpack_transform.scale), 0.0f, 10.0f);

            if (ImGui::DragFloat("shininess", &material_shininess, 1.0f)) {
                material_shininess = std::clamp(material_shininess, 1.0f, std::numeric_limits<float>::max());
            }
        ImGui::End();

        _imgui_frame_end();
    #pragma endregion ImGui

        glfwSwapBuffers(m_window);
    }
}

application::~application() {
    _destroy();
    _imgui_shutdown();
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

    OGL_CALL(glViewport(app->m_proj_settings.x, app->m_proj_settings.y, width, height));

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
