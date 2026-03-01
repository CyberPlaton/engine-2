#include <engine/shaders/bgfx_shader.sh>
#include <engine/shaders/shaderlib.sh>

uniform vec4 u_dt_alpha_discard_reference;
#define u_dt u_dt_alpha_discard_reference.x
#define u_alpha_reference u_dt_alpha_discard_reference.y
#define u_animation_time u_dt_alpha_discard_reference.z