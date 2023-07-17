#version 430 core

out vec4 frag_color;

in vec3 in_frag_pos;
in vec3 in_normal;
in vec2 in_texcoord;

struct light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 position;
    vec3 direction;
    float cutoff;
};

uniform light u_light;

struct material {
	sampler2D diffuse;
	sampler2D emission;
	sampler2D specular;
	float shininess;
};
uniform material u_material;

uniform double u_time;

void main() {
    const vec3 light_dir = normalize(in_frag_pos - u_light.position);
    const vec3 view_dir = normalize(u_light.direction);

    const vec3 diffuse_map = texture(u_material.diffuse, in_texcoord).rgb;
    const vec3 specular_map = texture(u_material.specular, in_texcoord).rgb;
    
    const vec3 ambient = u_light.ambient * diffuse_map;
    const vec3 emission = texture(u_material.emission, (vec2(in_texcoord.x, (1.0 - in_texcoord.y) - u_time))).rgb * floor(vec3(1.0) - specular_map);

    const float theta = dot(light_dir, view_dir);
    if (theta > u_light.cutoff) {
        const vec3 normal = normalize(in_normal);
        const float diff = max(dot(normal, -light_dir), 0.0);
        const vec3 diffuse = diff * u_light.diffuse * diffuse_map;

        const vec3 reflect_dir = reflect(light_dir, normal);

        const float spec = pow(max(dot(-view_dir, reflect_dir), 0.0), u_material.shininess);
        const vec3 specular = spec * u_light.specular * specular_map;  

        frag_color = vec4(ambient + diffuse + specular + emission, 1.0);
    } else {
        frag_color = vec4(ambient + emission, 1.0);
    }
}