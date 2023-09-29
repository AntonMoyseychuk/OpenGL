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

    const glm::vec3 camera_position(-25.0f, 500.0f, 55.0f);
    m_camera.create(camera_position, camera_position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 300.0f, 10.0f);

    m_proj_settings.x = m_proj_settings.y = 0;
    m_proj_settings.near = 0.1f;
    m_proj_settings.far = 2200.0f;
    _window_resize_callback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, &_window_resize_callback);

    m_renderer.enable(GL_CLIP_DISTANCE0);
}

mesh generate_water_mesh(uint32_t width, uint32_t depth, float height) noexcept {
    std::vector<mesh::vertex> vertices(width * depth);
    
    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            mesh::vertex& vertex = vertices[z * width + x];
            vertex.position = glm::vec3(x, height, z);
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texcoord = glm::vec2(x / static_cast<float>(width - 1), (depth - 1 - z) / static_cast<float>(depth - 1));
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve((width - 1) * (depth - 1) * 6);
    for (uint32_t z = 0; z < depth - 1; ++z) {
        for (uint32_t x = 0; x < width - 1; ++x) {
            indices.emplace_back(z * width + x);
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back(z * width + (x + 1));

            indices.emplace_back(z * width + (x + 1));
            indices.emplace_back((z + 1) * width + x);
            indices.emplace_back((z + 1) * width + (x + 1));
        }
    }

    return mesh(vertices, indices);
}

void application::run() noexcept {
    shader skybox_shader(RESOURCE_DIR "shaders/terrain/skybox.vert", RESOURCE_DIR "shaders/terrain/skybox.frag");
    shader shadow_shader(RESOURCE_DIR "shaders/terrain/shadowmap.vert", RESOURCE_DIR "shaders/terrain/shadowmap.frag");
    shader instanced_shadow_shader(RESOURCE_DIR "shaders/terrain/instacned_shadowmap.vert", RESOURCE_DIR "shaders/terrain/shadowmap.frag");
    shader terrain_shader(RESOURCE_DIR "shaders/terrain/terrain.vert", RESOURCE_DIR "shaders/terrain/terrain.frag");
    shader water_shader(RESOURCE_DIR "shaders/terrain/water.vert", RESOURCE_DIR "shaders/terrain/water.frag");
    shader plants_shader(RESOURCE_DIR "shaders/terrain/plants.vert", RESOURCE_DIR "shaders/terrain/plants.frag");

    framebuffer reflect_fbo;
    reflect_fbo.create();
    texture_2d reflect_color_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    reflect_color_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    reflect_color_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    reflect_color_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    reflect_color_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    reflect_fbo.attach(GL_COLOR_ATTACHMENT0, 0, reflect_color_buffer);
    renderbuffer reflect_depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);
    assert(reflect_fbo.attach(GL_DEPTH_ATTACHMENT, reflect_depth_buffer));

    framebuffer refract_fbo;
    refract_fbo.create();
    texture_2d refract_color_buffer(m_proj_settings.width, m_proj_settings.height, 0, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    refract_color_buffer.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    refract_color_buffer.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    refract_color_buffer.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    refract_color_buffer.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    refract_fbo.attach(GL_COLOR_ATTACHMENT0, 0, refract_color_buffer);
    renderbuffer refract_depth_buffer(m_proj_settings.width, m_proj_settings.height, GL_DEPTH_COMPONENT);
    assert(refract_fbo.attach(GL_DEPTH_ATTACHMENT, refract_depth_buffer));

    csm::shadowmap_config csm_config;
    csm_config.width = m_proj_settings.width;
    csm_config.height = m_proj_settings.height;
    csm_config.internal_format = GL_DEPTH_COMPONENT32F;
    csm_config.format = GL_DEPTH_COMPONENT;
    csm_config.type = GL_FLOAT;
    csm_config.mag_filter = GL_NEAREST;
    csm_config.min_filter = GL_NEAREST;
    csm_config.wrap_s = GL_CLAMP_TO_BORDER;
    csm_config.wrap_t = GL_CLAMP_TO_BORDER;
    csm_config.border_color = glm::vec4(1.0f);
    csm csm_shadowmap(3, csm_config);

    model cube(RESOURCE_DIR "models/cube/cube.obj", std::nullopt);
    cubemap skybox(std::array<std::string, 6>{
            RESOURCE_DIR "textures/terrain/skybox/right.png",
            RESOURCE_DIR "textures/terrain/skybox/left.png",
            RESOURCE_DIR "textures/terrain/skybox/top.png",
            RESOURCE_DIR "textures/terrain/skybox/bottom.png",
            RESOURCE_DIR "textures/terrain/skybox/front.png",
            RESOURCE_DIR "textures/terrain/skybox/back.png"
        }, false, false
    );
    skybox.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    skybox.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    skybox.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    skybox.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    terrain terrain(RESOURCE_DIR "textures/terrain/height_map.png", 10.0f, 6.0f);
    const std::vector<std::string> tile_textures = {
        RESOURCE_DIR "textures/terrain/sand_tile.jpg",
        RESOURCE_DIR "textures/terrain/grass_tile.png",
        RESOURCE_DIR "textures/terrain/rock_tile.jpg",
        RESOURCE_DIR "textures/terrain/snow_tile.png"
    };
    terrain.tiles.resize(tile_textures.size());
    for (size_t i = 0; i < tile_textures.size(); ++i) {
        terrain.tiles[i].texture.load(tile_textures[i], true, false, texture_2d::variety::NONE);
        terrain.tiles[i].texture.generate_mipmap();
        terrain.tiles[i].texture.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        terrain.tiles[i].texture.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        terrain.tiles[i].texture.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
        terrain.tiles[i].texture.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    terrain.calculate_tile_regions();

    const glm::mat4 terrain_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -500.0f, 0.0f));

    const float water_height = terrain.tiles[1].optimal;
    glm::vec4 default_clip_plane(0.0f, -1.0f, 0.0f, terrain.tiles.back().high + 1.0f);
    glm::vec4 refract_clip_plane(0.0f, -1.0f, 0.0f, water_height);
    glm::vec4 reflect_clip_plane(0.0f,  1.0f, 0.0f, -water_height);

    mesh water_mesh = generate_water_mesh(512, 512, water_height);
    const glm::mat4 water_model_matrix = terrain_model_matrix * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 1.0f, 5.0f));
    texture_2d dudv_map(RESOURCE_DIR "textures/terrain/dudv.png");
    dudv_map.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    dudv_map.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    dudv_map.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    dudv_map.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    texture_2d normal_map(RESOURCE_DIR "textures/terrain/water_normal_map.png");
    normal_map.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    normal_map.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    normal_map.set_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    normal_map.set_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);


    // model tree(RESOURCE_DIR "models/terrain/low_poly_tree.obj", std::nullopt);
    // texture_2d tree_surface(RESOURCE_DIR "textures/terrain/low_poly_tree.png");
    // tree_surface.set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // tree_surface.set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // tree_surface.set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // tree_surface.set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // tree_surface.generate_mipmap();
    //
    // std::vector<glm::mat4> tree_transforms(150);
    // for (size_t i = 0; i < tree_transforms.size(); ++i) {
    //     float x = std::numeric_limits<float>::lowest(), y = std::numeric_limits<float>::lowest(), z = std::numeric_limits<float>::lowest();
    //     while(glm::abs(terrain.tiles[1].optimal - y) >= 20.0f) {
    //         x = random<float>(5 * terrain.world_scale, (terrain.width - 5) * terrain.world_scale);
    //         z = random<float>(5 * terrain.world_scale, (terrain.depth - 5) * terrain.world_scale);
    //         y = terrain.get_interpolated_height(x, z);
    //     }
    //
    //     tree_transforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)) 
    //         * terrain_model_matrix * glm::scale(glm::mat4(1.0f), glm::vec3(1.2f));
    // }
    // buffer tree_transforms_buffer(
    //     GL_SHADER_STORAGE_BUFFER, tree_transforms.size() * sizeof(glm::mat4), sizeof(glm::mat4), GL_STATIC_DRAW, tree_transforms.data());

    
    glm::vec3 light_direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
    glm::vec3 light_color(0.74f, 0.396f, 0.055f);

    glm::vec3 fog_color = light_color;
    float fog_density = 0.00055f, fog_gradient = 4.5f;
    
    float skybox_speed = 0.01f;

    float wave_strength = 0.01f;
    float move_factor = 0.0f;
    float wave_speed = 0.02f;
    float water_shininess = 20.0f;
    float water_specular_strength = 0.5f;

    int32_t debug_cascade_index = 0;
    bool cascade_debug_mode = false;

    int32_t debug_terrain_tile_index = 0;
    int32_t debug_water_fbos_index = 0;

    ImGuiIO& io = ImGui::GetIO();

    auto render_scene = [&](bool shadow_pass = false) {
        const glm::mat4 skybox_model_matrix = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime() * skybox_speed, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 curr_light_direction = glm::normalize(glm::vec3(glm::transpose(glm::inverse(skybox_model_matrix)) * glm::vec4(light_direction, 0.0f)));

        const float z_mult = 2.1f;
        
        csm_shadowmap.calculate_subfrustas(
            glm::radians(m_camera.fov), 
            m_proj_settings.aspect, 
            m_proj_settings.near, 
            m_proj_settings.far, 
            m_camera.get_view(), 
            curr_light_direction,
            z_mult
        );

        for (size_t i = 0; i < csm_shadowmap.subfrustas.size(); ++i) {
            terrain_shader.uniform("u_light.csm.cascade_end_z[" + std::to_string(i) + "]", csm_shadowmap.subfrustas[i].far);
            plants_shader.uniform("u_light.csm.cascade_end_z[" + std::to_string(i) + "]", csm_shadowmap.subfrustas[i].far);
        }

        if (shadow_pass) {
            for (size_t i = 0; i < csm_shadowmap.subfrustas.size(); ++i) {
                m_renderer.viewport(0, 0, csm_shadowmap.shadowmaps[i].get_width(), csm_shadowmap.shadowmaps[i].get_height());

                csm_shadowmap.bind_for_writing(i);
                m_renderer.clear(GL_DEPTH_BUFFER_BIT);
                shadow_shader.uniform("u_projection", csm_shadowmap.subfrustas[i].lightspace_projection);
                shadow_shader.uniform("u_view", csm_shadowmap.subfrustas[i].lightspace_view);
                shadow_shader.uniform("u_model", terrain_model_matrix);
                m_renderer.render(GL_TRIANGLES, shadow_shader, terrain.ground_mesh);

                // instanced_shadow_shader.uniform("u_projection", csm_shadowmap.subfrustas[i].lightspace_projection);
                // instanced_shadow_shader.uniform("u_view", csm_shadowmap.subfrustas[i].lightspace_view);
                // tree_transforms_buffer.bind_base(0);
                // m_renderer.render_instanced(GL_TRIANGLES, instanced_shadow_shader, tree, tree_transforms.size());
            }
        } else {
            csm_shadowmap.bind_for_reading(10);
            for (size_t i = 0; i < csm_shadowmap.shadowmaps.size(); ++i) {
                const std::string csm_uniform = "u_light.csm.shadowmap[" + std::to_string(i) + "]";
                const std::string matrix_uniform = "u_light_space[" + std::to_string(i) + "]";
                const glm::mat4 light_space_matrix = csm_shadowmap.subfrustas[i].lightspace_projection * csm_shadowmap.subfrustas[i].lightspace_view;
                
                terrain_shader.uniform(csm_uniform, int32_t(10 + i));
                terrain_shader.uniform(matrix_uniform, light_space_matrix);

                // plants_shader.uniform(csm_uniform, int32_t(10 + i));
                // plants_shader.uniform(matrix_uniform, light_space_matrix);
            }

            skybox_shader.uniform("u_projection", m_proj_settings.projection_mat);
            skybox_shader.uniform("u_view", glm::mat4(glm::mat3(m_camera.get_view())));
            skybox_shader.uniform("u_model", skybox_model_matrix);
            skybox_shader.uniform("u_skybox", skybox, 0);

            terrain_shader.uniform("u_cascade_debug_mode", cascade_debug_mode);
            terrain_shader.uniform("u_projection", m_proj_settings.projection_mat);
            terrain_shader.uniform("u_model", terrain_model_matrix);

            terrain_shader.uniform("u_light.direction", curr_light_direction);
            terrain_shader.uniform("u_light.color", light_color);

            terrain_shader.uniform("u_fog.color", fog_color);
            terrain_shader.uniform("u_fog.density", fog_density);
            terrain_shader.uniform("u_fog.gradient", fog_gradient);

            terrain_shader.uniform("u_terrain.min_height", terrain.min_height);
            terrain_shader.uniform("u_terrain.max_height", terrain.max_height);
            for (size_t i = 0; i < terrain.tiles.size(); ++i) {
                terrain_shader.uniform("u_terrain.tiles[" + std::to_string(i) + "].texture", terrain.tiles[i].texture, 25 + i);
                terrain_shader.uniform("u_terrain.tiles[" + std::to_string(i) + "].low", terrain.tiles[i].low);
                terrain_shader.uniform("u_terrain.tiles[" + std::to_string(i) + "].optimal", terrain.tiles[i].optimal);
                terrain_shader.uniform("u_terrain.tiles[" + std::to_string(i) + "].high", terrain.tiles[i].high);
            }

            {
                reflect_fbo.bind();
                m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                m_renderer.viewport(0, 0, reflect_color_buffer.get_width(), reflect_color_buffer.get_height());

                m_renderer.disable(GL_CULL_FACE);
                m_renderer.render(GL_TRIANGLES, skybox_shader, cube);
                m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);

                const glm::vec3 origin_camera_position = m_camera.position;
                const glm::vec4 camera_terrain_position = glm::inverse(terrain_model_matrix) * glm::vec4(origin_camera_position, 1.0f);
                const float water_terrain_heigth = water_height;
                const float camera_to_water_dist = abs(camera_terrain_position.y - water_terrain_heigth);
                const glm::vec3 new_camera_position = origin_camera_position - glm::vec3(0.0f, 2.0f * camera_to_water_dist, 0.0f);
                m_camera.position = new_camera_position;
                m_camera.invert_pitch();
                terrain_shader.uniform("u_view", m_camera.get_view());
                m_camera.position = origin_camera_position;
                m_camera.invert_pitch();

                terrain_shader.uniform("u_water_clip_plane", reflect_clip_plane);
                m_renderer.enable(GL_CULL_FACE);
                m_renderer.render(GL_TRIANGLES, terrain_shader, terrain.ground_mesh);
                m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);
            }

            {
                refract_fbo.bind();
                m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                m_renderer.viewport(0, 0, refract_color_buffer.get_width(), refract_color_buffer.get_height());

                m_renderer.disable(GL_CULL_FACE);
                m_renderer.render(GL_TRIANGLES, skybox_shader, cube);
                m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);

                terrain_shader.uniform("u_view", m_camera.get_view());      
                terrain_shader.uniform("u_water_clip_plane", refract_clip_plane);  
                m_renderer.render(GL_TRIANGLES, terrain_shader, terrain.ground_mesh);
            }

            framebuffer::bind_default();
            m_renderer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_renderer.viewport(0, 0, m_proj_settings.width, m_proj_settings.height);
            
            m_renderer.disable(GL_CULL_FACE);
            m_renderer.render(GL_TRIANGLES, skybox_shader, cube);
            m_cull_face ? m_renderer.enable(GL_CULL_FACE) : m_renderer.disable(GL_CULL_FACE);

            terrain_shader.uniform("u_view", m_camera.get_view());
            terrain_shader.uniform("u_water_clip_plane", default_clip_plane);
            
            m_renderer.render(GL_TRIANGLES, terrain_shader, terrain.ground_mesh);

            water_shader.uniform("u_projection", m_proj_settings.projection_mat);
            water_shader.uniform("u_view", m_camera.get_view());
            water_shader.uniform("u_model", water_model_matrix);
            water_shader.uniform("u_reflection_map", reflect_color_buffer, 0);
            water_shader.uniform("u_refraction_map", refract_color_buffer, 1);
            water_shader.uniform("u_dudv_map", dudv_map, 2);
            water_shader.uniform("u_normal_map", normal_map, 3);
            water_shader.uniform("u_fog.color", fog_color);
            water_shader.uniform("u_fog.density", fog_density);
            water_shader.uniform("u_fog.gradient", fog_gradient);
            water_shader.uniform("u_light.direction", curr_light_direction);
            water_shader.uniform("u_light.color", light_color);
            water_shader.uniform("u_camera_position", m_camera.position);
            water_shader.uniform("u_wave_move_factor", move_factor);
            water_shader.uniform("u_wave_shininess", water_shininess);
            water_shader.uniform("u_specular_strength", water_specular_strength);
            water_shader.uniform("u_wave_strength", wave_strength);
            move_factor += wave_speed * io.DeltaTime;
            if (move_factor >= 1.0f) {
                move_factor -= 1.0f;
            }
            m_renderer.render(GL_TRIANGLES, water_shader, water_mesh);

            // plants_shader.uniform("u_cascade_debug_mode", cascade_debug_mode);
            // plants_shader.uniform("u_projection", m_proj_settings.projection_mat);
            // plants_shader.uniform("u_view", m_camera.get_view());
            //
            // plants_shader.uniform("u_light.direction", curr_light_direction);
            // plants_shader.uniform("u_light.color", light_color);
            //
            // plants_shader.uniform("u_fog.color", fog_color);
            // plants_shader.uniform("u_fog.density", fog_density);
            // plants_shader.uniform("u_fog.gradient", fog_gradient);
            //
            // plants_shader.uniform("u_material.diffuse0", tree_surface, 0);
            // tree_transforms_buffer.bind_base(0);
            // m_renderer.render_instanced(GL_TRIANGLES, plants_shader, tree, tree_transforms.size());
        }
    };

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

        render_scene(true);
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
                
            if (ImGui::NewLine(), ImGui::ColorEdit4("background color", glm::value_ptr(m_clear_color))) {
                m_renderer.set_clear_color(m_clear_color);
            }

            ImGui::Text("average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

    #if 0        
        ImGui::Begin("Water Framebuffers");
            ImGui::SliderInt(debug_water_fbos_index == 0 ? "refract" : "reflect", &debug_water_fbos_index, 0, 1);
            const uint32_t fbo_width = debug_water_fbos_index == 0 ? refract_color_buffer.get_width() : reflect_color_buffer.get_width();
            const uint32_t fbo_height = debug_water_fbos_index == 0 ? refract_color_buffer.get_height() : reflect_color_buffer.get_height();
            ImGui::Image((void*)(intptr_t)(debug_water_fbos_index == 0 ? refract_color_buffer.get_id() : reflect_color_buffer.get_id()), 
                ImVec2(fbo_width, fbo_height), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();

        ImGui::Begin("Terrain");
            ImGui::DragFloat("height scale", &terrain.height_scale, 0.001f, 0.01f, std::numeric_limits<float>::max());
            ImGui::DragFloat("world scale", &terrain.world_scale, 0.1f, 0.1f, std::numeric_limits<float>::max());

            if (ImGui::Button("regenerate")) {
                terrain.create(RESOURCE_DIR "textures/terrain/height_map.png", terrain.world_scale, terrain.height_scale);

                texture_2d surface(RESOURCE_DIR "textures/terrain/terrain_surface.png");
                terrain.mesh.add_texture(std::move(surface));
            }
        ImGui::End();
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
