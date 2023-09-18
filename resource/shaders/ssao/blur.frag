#version 460 core

out float frag_color;
  
in VS_OUT {
    vec2 texcoord;
} fs_in;
  
uniform sampler2D u_ssao_buffer;

void main() {
    const vec2 texel_size = 1.0f / textureSize(u_ssao_buffer, 0).xy;
    
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            const vec2 offset = vec2(x, y) * texel_size;
            result += texture(u_ssao_buffer, fs_in.texcoord + offset).r;
        }
    }
    
    frag_color = result / 16.0f;
}  