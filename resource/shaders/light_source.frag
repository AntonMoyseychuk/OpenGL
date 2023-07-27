#version 430 core

out vec4 out_color;

struct light_settings {
    vec3 color;
};

uniform light_settings u_light_settings;


void main() {
    out_color = vec4(u_light_settings.color, 1.0);
}