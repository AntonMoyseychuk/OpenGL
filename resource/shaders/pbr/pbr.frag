#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos_world;
    vec3 normal_world;
    vec2 texcoord;
} fs_in;

uniform struct pbr_material {
    sampler2D albedo_map;
    sampler2D normal_map;
    sampler2D metallic_map;
    sampler2D roughness_map;
    sampler2D ao_map;
} u_material;

const int LIGHTS_COUNT = 4;
uniform struct light {
    vec3 position;
    vec3 color;
} u_lights[LIGHTS_COUNT];

uniform vec3 u_camera_position;

const float PI = 3.14159265359f;

vec3 calculate_normal_from_map() {
    const vec3 tangent_normal = normalize(texture(u_material.normal_map, fs_in.texcoord).xyz * 2.0f - 1.0f);

    const vec3 Q1 = dFdx(fs_in.frag_pos_world);
    const vec3 Q2 = dFdy(fs_in.frag_pos_world);
    const vec2 st1 = dFdx(fs_in.texcoord);
    const vec2 st2 = dFdy(fs_in.texcoord);

    const vec3 N = normalize(fs_in.normal_world);
    const vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    const vec3 B = -normalize(cross(N, T));
    const mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}

float distribution_GGX(vec3 normal, vec3 half_direction, float roughness) {
    const float a = roughness*roughness;
    const float a2 = a * a;
    const float n_dot_h = max(dot(normal, half_direction), 0.0);
    const float n_dot_h2 = n_dot_h * n_dot_h;

    const float nom = a2;
    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_GGX(float n_dot_v, float roughness) {
    const float r = roughness + 1.0f;
    const float k = r * r / 8.0f;

    const float nom = n_dot_v;
    const float denom = n_dot_v * (1.0f - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 normal, vec3 view_direction, vec3 light_direction, float roughness) {
    float n_dot_v = max(dot(normal, view_direction), 0.0f);
    float n_dot_l = max(dot(normal, light_direction), 0.0f);
    float ggx2 = geometry_schlick_GGX(n_dot_v, roughness);
    float ggx1 = geometry_schlick_GGX(n_dot_l, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

void main() {		
    const vec3 view = normalize(u_camera_position - fs_in.frag_pos_world);
    const vec3 normal = calculate_normal_from_map();

    const vec3 albedo = texture(u_material.albedo_map, fs_in.texcoord).rgb;
    const float metallic = texture(u_material.metallic_map, fs_in.texcoord).r;
    const float roughness = texture(u_material.roughness_map, fs_in.texcoord).r;
    const float ao = texture(u_material.ao_map, fs_in.texcoord).r;

    vec3 F0 = vec3(0.04f); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0f);
    for(int i = 0; i < LIGHTS_COUNT; ++i) {
        const vec3 light_direction = normalize(u_lights[i].position - fs_in.frag_pos_world);
        const vec3 half_direction = normalize(view + light_direction);
        const float distance = length(u_lights[i].position - fs_in.frag_pos_world);
        const float attenuation = 1.0f / (distance * distance);
        const vec3 radiance = u_lights[i].color * attenuation;

        const float NDF = distribution_GGX(normal, half_direction, roughness);   
        const float G = geometry_smith(normal, view, light_direction, roughness);      
        const vec3 F = fresnel_schlick(clamp(dot(half_direction, view), 0.0, 1.0), F0);
           
        const vec3 numerator = NDF * G * F; 
        const float denominator = 4.0f * max(dot(normal, view), 0.0f) * max(dot(normal, light_direction), 0.0f) + 0.0001f;
        const vec3 specular = numerator / denominator;
        
        const vec3 kS = F;
        
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metallic;	  

        float NdotL = max(dot(normal, light_direction), 0.0f);        

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }   
    
    vec3 ambient = vec3(0.03f) * albedo * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f / 2.2f)); 

    frag_color = vec4(color, 1.0f);
}
