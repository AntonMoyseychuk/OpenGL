#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "assert.hpp"
#include "shader.hpp"
#include "texture.hpp"

std::unique_ptr<bool, application::glfw_deinitializer> application::is_glfw_initialized(new bool(glfwInit() == GLFW_TRUE));
application::framebuf_resize_controller& application::framebuffer = application::framebuf_resize_controller::get();

application &application::get(const std::string_view &title, uint32_t width, uint32_t height) noexcept {
    static application app(title, width, height);
    return app;
}

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title), m_camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f, 45.0f)
{
    const char* glfw_error_msg = nullptr;

    ASSERT(is_glfw_initialized != nullptr && *is_glfw_initialized, "GLFW error", 
        (glfwGetError(&glfw_error_msg), glfw_error_msg != nullptr ? glfw_error_msg : "unrecognized error"));

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

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
    ImGui_ImplOpenGL3_Init("#version 430");
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
    uint32_t VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    uint32_t VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // size_t indexes[] = {
    //     0, 2, 1,
    //     0, 3, 2
    // };
    //
    // uint32_t EBO;
    // glGenBuffers(1, &EBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

    const texture::config config(GL_TEXTURE_2D, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);

    texture texture0, texture1;
    texture0.create(RESOURCE_DIR "textures/minecraft_block.png", config);
    texture1.create(RESOURCE_DIR "textures/cat.png", config);

    shader shader;
    shader.create(RESOURCE_DIR "shaders/texture.vert", RESOURCE_DIR "shaders/texture.frag");
    shader.uniform("u_texture0", 0);
    shader.uniform("u_texture1", 1);

    shader.uniform("u_view", m_camera.get_view());
    shader.uniform("u_projection", framebuffer.projection);

    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwPollEvents();

        {
            if (glfwGetKey(m_window, GLFW_KEY_W)) {
                m_camera.move(m_camera.get_speed() * m_camera.get_forward() * io.DeltaTime);
                shader.uniform("u_view", m_camera.get_view());
            } else if (glfwGetKey(m_window, GLFW_KEY_S)) {
                m_camera.move(-m_camera.get_speed() * m_camera.get_forward() * io.DeltaTime);
                shader.uniform("u_view", m_camera.get_view());
            }
            if (glfwGetKey(m_window, GLFW_KEY_D)) {
                m_camera.move(m_camera.get_speed() * m_camera.get_right() * io.DeltaTime);
                shader.uniform("u_view", m_camera.get_view());
            } else if (glfwGetKey(m_window, GLFW_KEY_A)) {
                m_camera.move(-m_camera.get_speed() * m_camera.get_right() * io.DeltaTime);
                shader.uniform("u_view", m_camera.get_view());
            }

            if (glfwGetKey(m_window, GLFW_KEY_RIGHT)) {
                m_camera.rotate(-glm::radians(m_camera.get_sensitivity()) * io.DeltaTime, glm::vec2(0.0f, 1.0f));
                shader.uniform("u_view", m_camera.get_view());
            } else if (glfwGetKey(m_window, GLFW_KEY_LEFT)) {
                m_camera.rotate(glm::radians(m_camera.get_sensitivity()) * io.DeltaTime, glm::vec2(0.0f, 1.0f));
                shader.uniform("u_view", m_camera.get_view());
            }
            if (glfwGetKey(m_window, GLFW_KEY_UP)) {
                m_camera.rotate(-glm::radians(m_camera.get_sensitivity()) * io.DeltaTime, glm::vec2(1.0f, 0.0f));
                shader.uniform("u_view", m_camera.get_view());
            } else if (glfwGetKey(m_window, GLFW_KEY_DOWN)) {
                m_camera.rotate(glm::radians(m_camera.get_sensitivity()) * io.DeltaTime, glm::vec2(1.0f, 0.0f));
                shader.uniform("u_view", m_camera.get_view());
            }
        }

        shader.bind();
        shader.uniform("u_model", glm::rotate(glm::mat4(1.0f), (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f)));

        texture0.activate_unit(0);
        texture1.activate_unit(1);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // glDrawElements(GL_TRIANGLES, sizeof(indexes) / sizeof(indexes[0]), GL_UNSIGNED_INT, nullptr);

        {
            _imgui_frame_begin();

            ImGui::Begin("Information");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();

            ImGui::Begin("Camera");
            if (ImGui::DragFloat3("position", &m_camera.get_position().x) ||
                ImGui::SliderFloat("speed", &m_camera.get_speed(), 1.0f, 100.0f) ||
                ImGui::SliderFloat("sensitivity", &m_camera.get_sensitivity(), 1.0f, 100.0f)) {
                shader.uniform("u_view", m_camera.get_view());
            }
            if (ImGui::SliderFloat("field of view", &framebuffer.fov, 0.0f, 180.0f)) {
                framebuffer.on_resize(m_window, framebuffer.width, framebuffer.height);
                shader.uniform("u_projection", framebuffer.projection);
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
