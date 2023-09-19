#version 460 core

out float frag_color;

in VS_OUT {
    vec2 texcoord;
} fs_in;

struct GBuffer {
    sampler2D position;
    sampler2D normal;
};
uniform GBuffer u_gbuffer;

uniform mat4 u_projection;

uniform sampler2D u_noise_buffer;
uniform vec2 u_screen_size;
uniform float u_noise_scale = 4.0f;

uniform float u_radius = 0.5f;
uniform float u_bias = 0.025f;

const int MAX_KERNEL_SIZE = 128;
uniform vec3 u_kernel[MAX_KERNEL_SIZE];

void main() {
    // const vec2 noise_scale = u_screen_size / u_noise_scale;

    const vec3 frag_pos_view_space = texture(u_gbuffer.position, fs_in.texcoord).xyz;
    // const vec3 normal = normalize(texture(u_gbuffer.normal, fs_in.texcoord).xyz);
    // const vec3 random_offset = normalize(texture(u_noise_buffer, fs_in.texcoord * noise_scale).xyz);

    // const vec3 tangent = normalize(random_offset - normal * dot(random_offset, normal));
    // const vec3 bitangent = cross(normal, tangent);
    // const mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0f;
    for (int i = 0; i < MAX_KERNEL_SIZE; ++i) {
        // vec3 sampl = TBN * u_kernel[i];
        // sampl = frag_pos + sampl * u_radius;
        vec3 sampl = frag_pos_view_space + u_kernel[i];

        vec4 offset = u_projection * vec4(sampl, 1.0f);
        offset.xyz /= offset.w;
        offset.xyz = 0.5f * offset.xyz + 0.5f;

        const float sample_depth = texture(u_gbuffer.position, offset.xy).z;

        const float range_check = smoothstep(0.0f, 1.0f, u_radius / abs(frag_pos_view_space.z - sample_depth));

        if (sample_depth >= sampl.z + u_bias) {
            occlusion += range_check;
        }
    }

    occlusion = 1.0f - (occlusion / MAX_KERNEL_SIZE);

    frag_color = occlusion * occlusion;
}