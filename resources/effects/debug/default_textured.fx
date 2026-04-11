@ VERTEX
$input a_position, a_color0, a_texcoord0
$output v_color, v_texcoord0
#include <engine/shaders/bgfx_shader.sh>

//------------------------------------------------------------------------------------------------------------------------
void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
	v_color = a_color0;
	v_texcoord0 = a_texcoord0;
}
@ !VERTEX

@ PIXEL
$input v_color, v_texcoord0
#include <engine/shaders/bgfx_shader.sh>
SAMPLER2D(s_texture, 0);

//------------------------------------------------------------------------------------------------------------------------
void main()
{
	const vec4 texture_color = texture2D(s_texture, v_texcoord0);
	const vec4 color = v_color * texture_color;

	if (color.a < 0.5) { discard; }

	gl_FragColor = color;
}
@ !PIXEL