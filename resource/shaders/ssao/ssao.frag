#version 460 core

layout (location = 0) out float occlusion;

in VS_OUT {
    vec2 texcoord;
} fs_in;

uniform struct gbuffer {
    sampler2D position;
    sampler2D normal;
} u_gbuffer;

uniform sampler2D u_ssao_noise;

const int MAX_SAMPLES_COUNT = 128;
uniform uint u_samples_count = MAX_SAMPLES_COUNT;
uniform vec3 u_ssao_samples[MAX_SAMPLES_COUNT];

uniform vec2 u_screen_size;
uniform float u_noise_size = 4.0f;
uniform float u_noise_radius = 1.0f;
uniform float u_ssao_bias = 0.025f;

uniform mat4 u_projection_matrix;

void main() {
    const vec2 noise_scale = u_screen_size / u_noise_size;

    const vec3 position = texture(u_gbuffer.position, fs_in.texcoord).xyz;
    const vec3 normal = normalize(texture(u_gbuffer.normal, fs_in.texcoord).xyz);

    const vec3 random_vec = texture(u_ssao_noise, fs_in.texcoord * noise_scale).xyz;

    const vec3 N = normal;
    const vec3 T = normalize(random_vec - N * dot(random_vec, N));
    const vec3 B = cross(N, T);
    const mat3 TBN = mat3(T, B, N);

    occlusion = 0.0f;
    for (int i = 0; i < u_samples_count; ++i) {
        vec3 sampl = TBN * u_ssao_samples[i];
        sampl = position + sampl * u_noise_radius;

        vec4 offset = u_projection_matrix * vec4(sampl, 1.0f);
        offset = offset / offset.w;
        offset.xyz = offset.xyz * 0.5f + 0.5f;

        const float near_sample_depth = texture(u_gbuffer.position, offset.xy).z;

        const float range_check = smoothstep(0.0f, 1.0f, u_noise_radius / abs(position.z - near_sample_depth));
        occlusion += (near_sample_depth >= sampl.z + u_ssao_bias ? 1.0f : 0.0f) * range_check;
    }

    occlusion = 1.0f - (occlusion / u_samples_count);
    occlusion = occlusion * occlusion;
}