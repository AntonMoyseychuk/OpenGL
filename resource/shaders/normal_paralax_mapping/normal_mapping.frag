#version 430 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec2 texcoord;
    mat3 TBN;
} fs_in;

struct Material {
    sampler2D diffuse;
    sampler2D normal;
    sampler2D height;

    float shininess;
};
uniform Material u_material;

struct PointLight {
    vec3 position;
    vec3 color;

    float intensity;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight u_light;

uniform vec3 u_camera_position;
uniform float u_height_scale;

vec3 calc_normal(vec2 texcoord) {
    vec3 normal = texture(u_material.normal, texcoord).xyz;
    normal = 2.0f * normal - vec3(1.0f);
    return normalize(fs_in.TBN * normal);
}

vec2 parallax_mapping(vec2 tex_coords, vec3 view_dir) { 
    float height = texture(u_material.height, tex_coords).r;    
    vec2 p = view_dir.xy / view_dir.z * (height * u_height_scale);
    return tex_coords - p;    
} 

void main() {
    const vec3 view_direction = normalize(fs_in.frag_pos - u_camera_position);
    
    const vec2 texcoord = parallax_mapping(fs_in.texcoord, view_direction);
    if(texcoord.x > 1.0 || texcoord.y > 1.0 || texcoord.x < 0.0 || texcoord.y < 0.0) {
        discard;
    }
    
    const vec3 normal = calc_normal(texcoord);

    const vec3 albedo = texture(u_material.diffuse, texcoord).rgb;

    const vec3 ambient = 0.1f * albedo;

    const vec3 light_direction = normalize(fs_in.frag_pos - u_light.position);
    const float dist = length(fs_in.frag_pos - u_light.position);
        
    const vec3 half_direction = normalize(view_direction + light_direction);

    const float attenuation = u_light.intensity / (u_light.constant + u_light.linear * dist + u_light.quadratic * dist * dist);

    const float diff = max(dot(-light_direction, normal), 0.0f);
    const vec3 diffuse = diff * albedo * u_light.color * attenuation;

    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = spec * albedo * attenuation;

    frag_color = vec4(ambient + diffuse + specular, 1.0f);
}