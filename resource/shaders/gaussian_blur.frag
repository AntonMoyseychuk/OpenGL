#version 430 core

out vec4 frag_color;

in vec2 in_texcoord;

uniform sampler2D u_scene;

uniform bool u_horizontal;
uniform float u_weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {             
    const vec2 tex_offset = 1.0 / textureSize(u_scene, 0);
    
    vec3 result = texture(u_scene, in_texcoord).rgb * u_weight[0];
    if(u_horizontal) {
        for(int i = 1; i < 5; ++i) {
            result += texture(u_scene, in_texcoord + vec2(tex_offset.x * i, 0.0f)).rgb * u_weight[i];
            result += texture(u_scene, in_texcoord - vec2(tex_offset.x * i, 0.0f)).rgb * u_weight[i];
        }
    } else {
        for(int i = 1; i < 5; ++i) {
            result += texture(u_scene, in_texcoord + vec2(0.0f, tex_offset.y * i)).rgb * u_weight[i];
            result += texture(u_scene, in_texcoord - vec2(0.0f, tex_offset.y * i)).rgb * u_weight[i];
        }
    }

    frag_color = vec4(result, 1.0f);
}