@ PIXEL
$input v_texcoord0
#include <engine/shaders/common.sh>
SAMPLER2D(s_texture, 0);

//------------------------------------------------------------------------------------------------------------------------
void main()
{
	float texel = 1.0 / u_viewRect.w;
	vec3 result = vec3(0.0, 0.0, 0.0);
	result += texture2D(s_texture, v_texcoord0 + vec2(0.0, -2.0 * texel)).rgb * 0.0625;
	result += texture2D(s_texture, v_texcoord0 + vec2(0.0, -1.0 * texel)).rgb * 0.25;
	result += texture2D(s_texture, v_texcoord0).rgb                           * 0.375;
	result += texture2D(s_texture, v_texcoord0 + vec2(0.0,  1.0 * texel)).rgb * 0.25;
	result += texture2D(s_texture, v_texcoord0 + vec2(0.0,  2.0 * texel)).rgb * 0.0625;
	gl_FragColor = vec4(result, 1.0);
}
@ !PIXEL
