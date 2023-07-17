#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "assert.hpp"

#include "shader.hpp"
#include "texture.hpp"

#include <algorithm>

std::unique_ptr<bool, application::glfw_deinitializer> application::is_glfw_initialized(new bool(glfwInit() == GLFW_TRUE));
application::framebuf_resize_controller& application::framebuffer = application::framebuf_resize_controller::get();

application &application::get(const std::string_view &title, uint32_t width, uint32_t height) noexcept {
    static application app(title, width, height);
    return app;
}

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title), m_camera(glm::vec3(0.0f, 1.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 12.0f, 2.0f)
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

    m_window = glfwCreateWindow(width, height, m_title.c_str(), nullptr, nullptr);
    ASSERT(m_window != nullptr, "GLFW error", 
        (_destroy(), glfwGetError(&glfw_error_msg), glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error"));

    glfwMakeContextCurrent(m_window);
    ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "GLAD error", "failed to initialize GLAD");
    glfwSwapInterval(1);

    framebuffer.fov = 45.0f;
    framebuffer.near = 0.1f;
    framebuffer.far = 100.0f;

    framebuffer.on_resize(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &framebuf_resize_controller::on_resize);

    glEnable(GL_DEPTH_TEST);

    _init_imgui("#version 430");
}

application::~application() {
    _destroy();
    _shutdown_imgui();
}

void application::_destroy() noexcept {
    glfwDestroyWindow(m_window);
}

void application::_init_imgui(const char *glsl_version) const noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430 core");
}

void application::_shutdown_imgui() const noexcept {
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

void application::run() noexcept {
    float cube_vert[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    uint32_t obj_vao;
    glGenVertexArrays(1, &obj_vao);
    glBindVertexArray(obj_vao);

    uint32_t obj_vbo;
    glGenBuffers(1, &obj_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vert), cube_vert, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    uint32_t light_vao;
    glGenVertexArrays(1, &light_vao);
    glBindVertexArray(light_vao);

    uint32_t light_vbo;
    glGenBuffers(1, &light_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, light_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vert), cube_vert, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    const texture::config config(GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

    texture diffuse_map;
    diffuse_map.create(RESOURCE_DIR "textures/container.png", config);
    texture emission_map;
    emission_map.create(RESOURCE_DIR "textures/matrix.jpg", config);
    texture specular_map;
    specular_map.create(RESOURCE_DIR "textures/container_specular.png", config);

    shader directional_light_shader, point_light_shader, spot_light_shader;
    directional_light_shader.create(RESOURCE_DIR "shaders/light.vert", RESOURCE_DIR "shaders/directional_light.frag");
    directional_light_shader.uniform("u_material.diffuse", 0);
    directional_light_shader.uniform("u_material.specular", 1);
    directional_light_shader.uniform("u_material.emission", 2);
    point_light_shader.create(RESOURCE_DIR "shaders/light.vert", RESOURCE_DIR "shaders/point_light.frag");
    point_light_shader.uniform("u_material.diffuse", 0);
    point_light_shader.uniform("u_material.specular", 1);
    point_light_shader.uniform("u_material.emission", 2);
    spot_light_shader.create(RESOURCE_DIR "shaders/light.vert", RESOURCE_DIR "shaders/spot_light.frag");
    spot_light_shader.uniform("u_material.diffuse", 0);
    spot_light_shader.uniform("u_material.specular", 1);
    spot_light_shader.uniform("u_material.emission", 2);

    shader light_source_shader;
    light_source_shader.create(RESOURCE_DIR "shaders/light_source.vert", RESOURCE_DIR "shaders/light_source.frag");

    ImGuiIO& io = ImGui::GetIO();

    glm::vec3 point_light_color(1.0f), dir_light_color(1.0f), spot_light_color(1.0f), point_light_position(1.5f, 1.0f, 4.5f), light_direction(-1.0f, -1.0f, -1.0f);
    float material_shininess = 64.0f, cutoff = 12.5f;
    double linear_coef = 0.09, quadratic_coef = 0.032;

    std::vector<glm::vec3> transforms = {
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

    shader curr_shader = spot_light_shader;

    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();

        if (!m_fixed_camera) {
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

            if (glfwGetKey(m_window, GLFW_KEY_RIGHT)) {
                m_camera.rotate(-glm::radians(45.0f) * io.DeltaTime, glm::vec2(0.0f, 1.0f));
            } else if (glfwGetKey(m_window, GLFW_KEY_LEFT)) {
                m_camera.rotate(glm::radians(45.0f) * io.DeltaTime, glm::vec2(0.0f, 1.0f));
            }
            if (glfwGetKey(m_window, GLFW_KEY_UP)) {
                m_camera.rotate(-glm::radians(45.0f) * io.DeltaTime, glm::vec2(1.0f, 0.0f));
            } else if (glfwGetKey(m_window, GLFW_KEY_DOWN)) {
                m_camera.rotate(glm::radians(45.0f) * io.DeltaTime, glm::vec2(1.0f, 0.0f));
            }
        }

        diffuse_map.activate_unit(0);
        specular_map.activate_unit(1);
        emission_map.activate_unit(2);

        if (curr_shader == point_light_shader) {
            glBindVertexArray(light_vao);
            light_source_shader.bind();
            light_source_shader.uniform("u_model", glm::scale(glm::translate(glm::mat4(1.0f), point_light_position), glm::vec3(0.2f)));
            light_source_shader.uniform("u_view", m_camera.get_view());
            light_source_shader.uniform("u_projection", framebuffer.projection);
            light_source_shader.uniform("u_view_position", m_camera.position);
            light_source_shader.uniform("u_light_settings.color", point_light_color);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(obj_vao);
        curr_shader.bind();
        
        curr_shader.uniform("u_view", m_camera.get_view());
        curr_shader.uniform("u_projection", framebuffer.projection);
        curr_shader.uniform("u_view_position", m_camera.position);
        const double t = glfwGetTime();
        curr_shader.uniform("u_time", t - size_t(t));
        if (curr_shader == directional_light_shader) {
            curr_shader.uniform("u_light.direction", glm::normalize(light_direction));
            
            curr_shader.uniform("u_light.ambient", dir_light_color * glm::vec3(0.05f));
            curr_shader.uniform("u_light.diffuse", dir_light_color * glm::vec3(0.5f));
            curr_shader.uniform("u_light.specular", dir_light_color * glm::vec3(1.0f));
        } else if (curr_shader == point_light_shader) {
            curr_shader.uniform("u_light.position", point_light_position);

            curr_shader.uniform("u_light.linear", linear_coef);
            curr_shader.uniform("u_light.quadratic", quadratic_coef);
            
            curr_shader.uniform("u_light.ambient", point_light_color * glm::vec3(0.05f));
            curr_shader.uniform("u_light.diffuse", point_light_color * glm::vec3(0.5f));
            curr_shader.uniform("u_light.specular", point_light_color * glm::vec3(1.0f));
        } else if (curr_shader == spot_light_shader) {
            curr_shader.uniform("u_light.position", m_camera.position);
            curr_shader.uniform("u_light.direction", m_camera.get_forward());
            curr_shader.uniform("u_light.cutoff", glm::cos(glm::radians(cutoff)));

            curr_shader.uniform("u_light.ambient", spot_light_color * glm::vec3(0.05f));
            curr_shader.uniform("u_light.diffuse", spot_light_color * glm::vec3(0.5f));
            curr_shader.uniform("u_light.specular", spot_light_color * glm::vec3(1.0f));
        }
        curr_shader.uniform("u_material.shininess", material_shininess);
        
        for (size_t i = 0; i < transforms.size(); ++i) {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), transforms[i]);
            curr_shader.uniform("u_model", model);
            curr_shader.uniform("u_transp_inv_model", glm::transpose(glm::inverse(model)));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        {
            _imgui_frame_begin();

            ImGui::Begin("Information");
                if (ImGui::Checkbox("wireframe mode", &m_wireframed)) {
                    glPolygonMode(GL_FRONT_AND_BACK, (m_wireframed ? GL_LINE : GL_FILL));
                }
                ImGui::Checkbox("fixed camera mode", &m_fixed_camera);
                ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();

            ImGui::Begin("Camera");
                if (ImGui::DragFloat3("position", glm::value_ptr(m_camera.position), 0.1f) ||
                    ImGui::DragFloat("speed", &m_camera.speed, 0.1f) ||
                    ImGui::DragFloat("sensitivity", &m_camera.sensitivity, 0.1f)
                ) {
                    m_camera.speed = std::clamp(m_camera.speed, 1.0f, std::numeric_limits<float>::max());
                    m_camera.sensitivity = std::clamp(m_camera.sensitivity, 0.1f, std::numeric_limits<float>::max());
                }
                if (ImGui::SliderFloat("field of view", &framebuffer.fov, 1.0f, 179.0f)) {
                    framebuffer.on_resize(m_window, framebuffer.width, framebuffer.height);
                }
            ImGui::End();

            ImGui::Begin("Light");
                ImGui::Text("type: "); 
                if (curr_shader == directional_light_shader) {
                    ImGui::SameLine(); ImGui::TextColored(ImColor(0.0f, 1.0f, 0.0f), "directional");

                    ImGui::NewLine(); ImGui::DragFloat3("direction", glm::value_ptr(light_direction), 0.01f);
                    ImGui::ColorEdit3("color", glm::value_ptr(dir_light_color));
                } else if (curr_shader == point_light_shader) {
                    ImGui::SameLine(); ImGui::TextColored(ImColor(0.0f, 1.0f, 0.0f), "point");

                    ImGui::NewLine(); ImGui::DragFloat3("position", glm::value_ptr(point_light_position), 0.1f);
                    ImGui::InputDouble("linear", &linear_coef);
                    ImGui::InputDouble("quadratic", &quadratic_coef);
                    ImGui::ColorEdit3("color", glm::value_ptr(point_light_color));
                } else if (curr_shader == spot_light_shader) {
                    ImGui::SameLine(); ImGui::TextColored(ImColor(0.0f, 1.0f, 0.0f), "spot");
                    
                    ImGui::NewLine(); ImGui::SliderFloat("cutoff", &cutoff, 0.0f, 180.0f);
                    ImGui::ColorEdit3("color", glm::value_ptr(spot_light_color));
                }
            ImGui::End();

            ImGui::Begin("Plane");
                if (ImGui::DragFloat("shininess", &material_shininess, 1.0f)) {
                    material_shininess = std::clamp(material_shininess, 1.0f, std::numeric_limits<float>::max());
                }
            ImGui::End();

            _imgui_frame_end();
        }

        glfwSwapBuffers(m_window);
    }
}

void application::glfw_deinitializer::operator()(bool *is_glfw_initialized) noexcept {
    glfwTerminate();
    if (is_glfw_initialized != nullptr) {
        *is_glfw_initialized = false;
    }
}

application::framebuf_resize_controller::framebuf_resize_controller(float fov, float aspect, float near, float far)
    : fov(fov), aspect(aspect), near(near), far(far), projection(glm::perspective(glm::radians(fov), aspect, near, far)) 
{
}

application::framebuf_resize_controller &application::framebuf_resize_controller::get() noexcept {
    static framebuf_resize_controller state;
    return state;
}

void application::framebuf_resize_controller::on_resize(GLFWwindow *window, int32_t width, int32_t height) noexcept {
    glViewport(0, 0, width, height);

    framebuffer.width = width;
    framebuffer.height = height;
    
    framebuffer.aspect = static_cast<float>(width) / height;
    
    framebuffer.projection = glm::perspective(glm::radians(framebuffer.fov), framebuffer.aspect, framebuffer.near, framebuffer.far);
}
