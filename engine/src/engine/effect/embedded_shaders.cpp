#include <engine/effect/embedded_shaders.hpp>
#include <unordered_map>

namespace kokoro
{
	namespace
	{
		static std::unordered_map<const char*, const char*> S_SHADERS =
		{
			//- Default vertex shader used for postprocessing shaders
			//------------------------------------------------------------------------------------------------------------------------
			{"vs_postprocess",

			"@ VERTEX								\n"
			"$input a_position, a_texcoord0			\n"
			"$output v_texcoord0					\n"
			"#include </shaders/common.sh>			\n"
			"void main()							\n"
			"{										\n"
			"\tgl_Position = vec4(a_position, 1.0);	\n"
			"\tv_texcoord0 = a_texcoord0;			\n"
			"}										\n"
			"@ !VERTEX								\n"
			},

			//- Default/Internal pixel shader used to draw final combined texture to backbuffer
			//------------------------------------------------------------------------------------------------------------------------
			{"ps_combine",

			"@ PIXEL											\n"
			"$input v_texcoord0									\n"
			"#include </shaders/common.sh>						\n"
			"SAMPLER2D(s_texture, 0);							\n"
			"void main()										\n"
			"{													\n"
			"\tgl_FragColor = texture2D(s_texture, vec2(v_texcoord0.x, 1.0 - v_texcoord0.y));\n"
			"}													\n"
			"@ !PIXEL											\n"
			},

			//- Default/Internal pixel shader used to draw postprocessing chain texture back to input texture,
			//- here we expect that the postprocess count was i % 2 == 0
			//------------------------------------------------------------------------------------------------------------------------
			{"ps_postprocess_apply_back0",

			"@ PIXEL											\n"
			"$input v_texcoord0									\n"
			"#include </shaders/common.sh>						\n"
			"SAMPLER2D(s_texture, 0);							\n"
			"void main()										\n"
			"{													\n"
			"\tgl_FragColor = texture2D(s_texture, vec2(v_texcoord0.x, 1.0 - v_texcoord0.y));\n"
			"}													\n"
			"@ !PIXEL											\n"
			},

			//- Default/Internal pixel shader used to draw postprocessing chain texture back to input texture,
			//- here we expect that the postprocess count was i % 2 != 0
			//------------------------------------------------------------------------------------------------------------------------
			{"ps_postprocess_apply_back1",

			"@ PIXEL											\n"
			"$input v_texcoord0									\n"
			"#include </shaders/common.sh>						\n"
			"SAMPLER2D(s_texture, 0);							\n"
			"void main()										\n"
			"{													\n"
			"\tgl_FragColor = texture2D(s_texture, v_texcoord0);\n"
			"}													\n"
			"@ !PIXEL											\n"
			},


			//- Default vertex shader used to draw sprites
			//------------------------------------------------------------------------------------------------------------------------
			{"vs_default",

			"@ VERTEX												\n"
			"$input a_position, a_color0, a_texcoord0				\n"
			"$output v_color0, v_texcoord0							\n"
			"#include </shaders/common.sh>							\n"
			"void main()											\n"
			"{														\n"
			"\tgl_Position = mul(u_viewProj, vec4(a_position, 1.0));\n"
			"\tv_color0 = a_color0;									\n"
			"\tv_texcoord0 = a_texcoord0;							\n"
			"}														\n"
			"@ !VERTEX												\n"
			},

			//- Default pixel shader used to draw sprites
			//------------------------------------------------------------------------------------------------------------------------
			{"ps_default",

			"@ PIXEL														\n"
			"$input v_color0, v_texcoord0									\n"
			"#include </shaders/common.sh>									\n"
			"SAMPLER2D(s_texture, 0);										\n"
			"void main()													\n"
			"{																\n"
			"\tvec4 color = v_color0 * texture2D(s_texture, v_texcoord0);	\n"
			"\tif (color.w < 1.0 / 255.0) {									\n"
			"\t\tdiscard;													\n"
			"\t}															\n"
			"\tgl_FragColor = color;										\n"
			"}																\n"
			"@ !PIXEL														\n"
			},

			//- Example postprocessing shader that applies sepia tint to final texture
			//------------------------------------------------------------------------------------------------------------------------
			{"ps_sepia",

			"@ PIXEL													\n"
			"$input v_texcoord0											\n"
			"#include </shaders/common.sh>								\n"
			"SAMPLER2D(s_texture, 0);									\n"
			"void main()												\n"
			"{															\n"
			"\tvec4 color = texture2D(s_texture, v_texcoord0);			\n"
			"\tvec3 sepia = vec3(										\n"
			"\t\tcolor.r * 0.393 + color.g * 0.769 + color.b * 0.189,	\n"
			"\t\tcolor.r * 0.349 + color.g * 0.686 + color.b * 0.168,	\n"
			"\t\tcolor.r * 0.272 + color.g * 0.534 + color.b * 0.131	\n"
			"\t);														\n"
			"\tgl_FragColor = vec4(sepia, color.a);						\n"
			"}															\n"
			"@ !PIXEL													\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{"ps_blur",

			"@ PIXEL																			\n"
			"$input v_texcoord0																	\n"
			"#include </shaders/common.sh>														\n"
			"SAMPLER2D(s_texture, 0);															\n"
			"void main()																		\n"
			"{																					\n"
			"\tvec2 texelSize = vec2(1.0 / u_viewRect.z, 1.0 / u_viewRect.w);					\n"
			"\tvec4 color = texture2D(s_texture, v_texcoord0) * 4.0;							\n"
			"\tcolor += texture2D(s_texture, v_texcoord0 + texelSize.xy);						\n"
			"\tcolor += texture2D(s_texture, v_texcoord0 - texelSize.xy);						\n"
			"\tcolor += texture2D(s_texture, v_texcoord0 + vec2(texelSize.x, -texelSize.y));	\n"
			"\tcolor += texture2D(s_texture, v_texcoord0 - vec2(texelSize.x, -texelSize.y));	\n"
			"\tgl_FragColor = color / 8.0;														\n"
			"}																					\n"
			"@ !PIXEL																			\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{"ps_grayscale",

			"@ PIXEL												\n"
			"$input v_texcoord0										\n"
			"#include </shaders/common.sh>							\n"
			"SAMPLER2D(s_texture, 0);								\n"
			"void main()											\n"
			"{														\n"
			"\tvec3 color = texture2D(s_texture, v_texcoord0).rgb;	\n"
			"\tfloat gray = dot(color, vec3(0.299, 0.587, 0.114));	\n"
			"\tgl_FragColor = vec4(gray, gray, gray, 1.0);			\n"
			"}														\n"
			"@ !PIXEL												\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_vignette",

			"@ PIXEL												\n"
			"$input v_texcoord0										\n"
			"#include </shaders/common.sh>							\n"
			"SAMPLER2D(s_texture, 0);								\n"
			"void main()											\n"
			"{														\n"
			"\tvec2 uv = v_texcoord0;								\n"
			"\tvec2 center = vec2(0.5, 0.5);						\n"
			"\tfloat dist = length(uv - center);					\n"
			"\tfloat vignette = smoothstep(0.8, 0.5, dist * 1.2);	\n"
			"\tvec3 color = texture2D(s_texture, uv).rgb * vignette;\n"
			"\tgl_FragColor = vec4(color, 1.0);						\n"
			"}														\n"
			"@ !PIXEL												\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_filmgrain",

			"@ PIXEL															\n"
			"$input v_texcoord0													\n"
			"#include </shaders/common.sh>										\n"
			"SAMPLER2D(s_texture, 0);											\n"
			"float hash(vec2 p) {												\n"
			"\tp = fract(p * vec2(123.34, 456.21));								\n"
			"\tp += dot(p, p + 78.233);											\n"
			"\treturn fract(p.x * p.y);											\n"
			"}																	\n"
			"float random(vec2 uv, float seed) {								\n"
			"\treturn hash(uv + vec2(seed * 100.0, seed * 100.0));\n"
			"}																	\n"
			"void main()														\n"
			"{																	\n"
			"\tvec2 uv = v_texcoord0;											\n"
			"\tvec3 color = texture2D(s_texture, uv).rgb;						\n"
			"\tfloat noise = (random(uv * 100.0, fract(u_animation_time * 10.0)) - 0.5) * 0.1;\n"
			"\tcolor += noise;													\n"
			"\tgl_FragColor = vec4(color, 1.0);									\n"
			"}																	\n"
			"@ !PIXEL															\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_chromatic_aberration",

			"@ PIXEL											\n"
			"$input v_texcoord0									\n"
			"#include </shaders/common.sh>						\n"
			"SAMPLER2D(s_texture, 0);							\n"
			"void main()										\n"
			"{													\n"
			"\tvec2 uv = v_texcoord0;							\n"
			"\tvec2 offset = 0.005 * (uv - vec2(0.5, 0.5));		\n"
			"\tfloat r = texture2D(s_texture, uv + offset).r;	\n"
			"\tfloat g = texture2D(s_texture, uv).g;			\n"
			"\tfloat b = texture2D(s_texture, uv - offset).b;	\n"
			"\tgl_FragColor = vec4(r, g, b, 1.0);				\n"
			"}													\n"
			"@ !PIXEL											\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_bloom",

			"@ PIXEL													\n"
			"$input v_texcoord0											\n"
			"#include </shaders/common.sh>								\n"
			"SAMPLER2D(s_texture, 0);									\n"
			"void main()												\n"
			"{															\n"
			"\tvec3 color = texture2D(s_texture, v_texcoord0).rgb;		\n"
			"\tfloat brightness = dot(color, vec3(0.299, 0.587, 0.114));\n"
			"\tif (brightness > 0.7) { color *= 1.5; }					\n"
			"\tgl_FragColor = vec4(color, 1.0);							\n"
			"}															\n"
			"@ !PIXEL													\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_scanlines",

			"@ PIXEL														\n"
			"$input v_texcoord0												\n"
			"#include </shaders/common.sh>									\n"
			"SAMPLER2D(s_texture, 0);										\n"
			"void main()													\n"
			"{																\n"
			"\tvec2 uv = v_texcoord0;										\n"
			"\tfloat scanline = sin(uv.y * u_viewRect.z) * 0.1;				\n"
			"\tvec3 color = texture2D(s_texture, uv).rgb * (1.0 - scanline);\n"
			"\tgl_FragColor = vec4(color, 1.0);								\n"
			"}																\n"
			"@ !PIXEL														\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_sharpen",

			"@ PIXEL																\n"
			"$input v_texcoord0														\n"
			"#include </shaders/common.sh>											\n"
			"SAMPLER2D(s_texture, 0);												\n"
			"void main()															\n"
			"{																		\n"
			"\tvec2 texel = vec2(1.0 / u_viewRect.z, 1.0 / u_viewRect.w);			\n"
			"\t																		\n"
			"\tvec3 orig = texture2D(s_texture, v_texcoord0).rgb;					\n"
			"\tvec3 blur = (														\n"
			"\t\ttexture2D(s_texture, v_texcoord0 + texel).rgb +					\n"
			"\t\ttexture2D(s_texture, v_texcoord0 - texel).rgb +					\n"
			"\t\ttexture2D(s_texture, v_texcoord0 + vec2(texel.x, -texel.y)).rgb +	\n"
			"\t\ttexture2D(s_texture, v_texcoord0 - vec2(texel.x, -texel.y)).rgb	\n"
			"\t) * 0.25;															\n"
			"\t																		\n"
			"\tvec3 sharpened = orig + (orig - blur) * 1.5;							\n"
			"\tgl_FragColor = vec4(sharpened, 1.0);									\n"
			"}																		\n"
			"@ !PIXEL																\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_invert",

			"@ PIXEL												\n"
			"$input v_texcoord0										\n"
			"#include </shaders/common.sh>							\n"
			"SAMPLER2D(s_texture, 0);								\n"
			"void main()											\n"
			"{														\n"
			"\tvec3 color = texture2D(s_texture, v_texcoord0).rgb;	\n"
			"\tgl_FragColor = vec4(1.0 - color, 1.0);				\n"
			"}														\n"
			"@ !PIXEL												\n"
			},

			//------------------------------------------------------------------------------------------------------------------------
			{ "ps_posterize",

			"@ PIXEL												\n"
			"$input v_texcoord0										\n"
			"#include </shaders/common.sh>							\n"
			"SAMPLER2D(s_texture, 0);								\n"
			"void main()											\n"
			"{														\n"
			"\tvec3 color = texture2D(s_texture, v_texcoord0).rgb;	\n"
			"\tcolor = floor(color * 4.0) / 4.0;					\n"
			"\tgl_FragColor = vec4(color, 1.0);						\n"
			"}														\n"
			"@ !PIXEL												\n"
			}
		};

	} //- unnamed

	//------------------------------------------------------------------------------------------------------------------------
	const char* sembedded_shaders::get(type type_name)
	{
		static const char* S_NAMES[] =
		{
#define ENTRY(name) #name,
			KOKORO_EMBEDDED_SHADERS_LIST
#undef ENTRY
		};
		return get(S_NAMES[static_cast<int>(type_name)]);
	}

	//------------------------------------------------------------------------------------------------------------------------
	const char* sembedded_shaders::get(const char* name)
	{
		return S_SHADERS.at(name);
	}

} //- kokoro