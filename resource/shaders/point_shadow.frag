#version 430 core

in GEOM_OUT {
    vec4 frag_pos;
} fs_in;
 
uniform vec3 u_light_pos;
uniform float u_far_plane;
 
void main()
{
    // Получаем расстояние межу фрагментом и источником света
    float lightDistance = length(fs_in.frag_pos.xyz - u_light_pos);
    
    // Приводим значение к диапазону [0;1] путем деления на far_plane
    lightDistance = lightDistance / u_far_plane;
    
    // Записываем его в качестве измененной глубины
    gl_FragDepth = lightDistance;
}  