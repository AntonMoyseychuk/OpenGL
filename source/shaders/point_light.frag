#version 430 core

out vec4 frag_color;

in vec3 in_frag_pos;
in vec3 in_normal;
in vec2 in_texcoord;

struct light {
	vec3 position;

	double linear;
	double quadratic;

	vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform light u_light;

struct material {
	sampler2D diffuse;
	sampler2D emission;
	sampler2D specular;
	float shininess;
};
uniform material u_material;

uniform vec3 u_view_position;

uniform double u_time;

void main() {
	const double dist = length(in_frag_pos - u_light.position);
	const double attenuation = 1.0 / (1.0 + u_light.linear * dist + u_light.quadratic * (dist * dist));
	
	const vec3 diffuse_map = texture(u_material.diffuse, in_texcoord).rgb;
    const vec3 ambient = u_light.ambient * diffuse_map;

	const vec3 normal = normalize(in_normal);
	const vec3 light_dir = normalize(in_frag_pos - u_light.position);
	const float diff = max(dot(normal, -light_dir), 0.0);
	const vec3 diffuse = diff * u_light.diffuse * diffuse_map;

	const vec3 view_dir = normalize(in_frag_pos - u_view_position);
	const vec3 reflect_dir = reflect(light_dir, normal);

	const vec3 specular_map = texture(u_material.specular, in_texcoord).rgb;

	const float spec = pow(max(dot(-view_dir, reflect_dir), 0.0), u_material.shininess);
	const vec3 specular = spec * u_light.specular * specular_map;  

	const vec3 emission = texture(u_material.emission, (vec2(in_texcoord.x, (1.0 - in_texcoord.y) - u_time))).rgb * floor(vec3(1.0) - specular_map);

	frag_color = vec4((ambient + diffuse + specular) * attenuation + emission, 1.0);
}