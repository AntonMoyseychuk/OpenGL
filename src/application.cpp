#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>

#include "assert_macro.hpp"

std::unique_ptr<bool, application::glfw_deinitializer> application::is_glfw_initialized(new bool(glfwInit() == GLFW_TRUE));

application &application::get(const std::string_view &title, uint32_t width, uint32_t height) noexcept {
    static application app(title, width, height);
    return app;
}

application::application(const std::string_view &title, uint32_t width, uint32_t height)
    : m_title(title)
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

    _framebuffer_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &application::_framebuffer_resize_callback);
}

application::~application() {
    _destroy();
}

void application::_destroy() noexcept {
    glfwDestroyWindow(m_window);
}

void application::_framebuffer_resize_callback(GLFWwindow *window, int32_t width, int32_t height) noexcept {
    glViewport(0, 0, width, height);
}

void application::run() noexcept {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 position;
        void main() {
            gl_Position = vec4(position.x, position.y, position.z, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 color;
        void main() {
            color = vec4(1.0, 0.0, 0.5, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLfloat vertices[] = {
       -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();

        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Information");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(m_window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void application::glfw_deinitializer::operator()(bool *is_glfw_initialized) noexcept {
    glfwTerminate();
    if (is_glfw_initialized != nullptr) {
        *is_glfw_initialized = false;
    }
}
