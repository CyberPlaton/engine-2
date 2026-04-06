@ PIXEL
$input v_texcoord0
#include <engine/shaders/common.sh>
SAMPLER2D(s_texture, 0);
SAMPLER2D(s_bloom,   1);

//------------------------------------------------------------------------------------------------------------------------
void main()
{
	vec3 scene = texture2D(s_texture, v_texcoord0).rgb;
	vec3 bloom = texture2D(s_bloom,   v_texcoord0).rgb;
	gl_FragColor = vec4(scene + bloom * 1.5, 1.0);
}
@ !PIXEL
