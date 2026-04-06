@ PIXEL
$input v_texcoord0
#include <engine/shaders/common.sh>
SAMPLER2D(s_texture, 0);

//------------------------------------------------------------------------------------------------------------------------
void main()
{
	vec3 color = texture2D(s_texture, v_texcoord0).rgb;
	float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float soft = clamp((brightness - 0.6) / 0.3, 0.0, 1.0);
	gl_FragColor = vec4(color * soft, 1.0);
}
@ !PIXEL
