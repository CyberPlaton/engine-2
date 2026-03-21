IS_PROFILE_ENABLED				= true	-- Enable profiling, either with in-engine or external tools
IS_TRACY_ENABLED				= true	-- Enable profiling with Tracy
IS_LOGGING_ENABLED				= true	-- Enable log output
IS_WARNING_AND_ERRORS_ENABLED	= false	-- Enable compiler warnings and warning as errors messages
IS_ECS_DEBUG_INFO_ENABLED		= true	-- Enabled gathering debug information about modules, systems, entities and their components etc.
IS_ADRESS_SANITIZE_ENABLED		= false	-- Enabled adress sanitizer for deeper checking. Available for windows.
IS_SIMD_ENABLED					= true	-- Enabled usage of SIMD on available platforms

------------------------------------------------------------------------------------------------------------------------
function isempty(s)
	return s == nil or s == ''
end

------------------------------------------------------------------------------------------------------------------------
function tables_concat(a, b)
	for _, v in ipairs(b) do
        table.insert(a, v)
    end
end

------------------------------------------------------------------------------------------------------------------------
function set_and_disable_common_warnings_errors()
		filter {"action:vs*"}
			buildoptions{"/wd4005"}						-- disable "macro redefined" warning
		filter{"action:gmake*", "action:clang*"}
			buildoptions{"-Wno-builtin-macro-redefined", "-Wno-macro-redefined"}
		filter{}
end

------------------------------------------------------------------------------------------------------------------------
function set_project_resources_dir()
	local curr = os.getcwd()
	local res_dir = path.join(curr, "resources")
	defines { "PROJECT_RESOURCES_DIR=\"" .. res_dir .. "\"" }
end

------------------------------------------------------------------------------------------------------------------------
function set_engine_resources_dir()
	local res_dir = path.join(WORKSPACE_DIR, "resources")
	defines { "KOKORO_RESOURCES_DIR=\"" .. res_dir .. "\"" }
end

------------------------------------------------------------------------------------------------------------------------
function set_basic_defines()
	externalanglebrackets "On"

	if PLATFORM == "windows" then
		defines { "BGFX_CONFIG_RENDERER_DIRECT3D11", "PLATFORM_WINDOWS=1", "NODRAWTEXT", "WIN32_LEAN_AND_MEAN", "NOMINMAX"}
		buildoptions{"/bigobj"}
		editandcontinue "Off"
		filter {"action:vs*"}
			buildoptions{"/Zc:__cplusplus", "/Zc:preprocessor"}
			if IS_ADRESS_SANITIZE_ENABLED == true then
				buildoptions{"/fsanitize=address"}
			end
		filter{}
	elseif PLATFORM == "linux" then
		defines { "BGFX_CONFIG_RENDERER_VULKAN", "PLATFORM_LINUX=1"}
		filter {"action:gmake*"}
			buildoptions{"-fPIC"}
		filter{}
	elseif PLATFORM == "macosx" then
		defines {"BGFX_CONFIG_RENDERER_VULKAN", "PLATFORM_MACOSX=1"}
	else
	end

	if IS_PROFILE_ENABLED == true then
		defines{"PROFILE_ENABLE"}
	end
	if IS_TRACY_ENABLED == true then
		-- Enable tracy and track data only if a server (viewer application) is connected
		defines{"TRACY_ENABLE", "TRACY_ON_DEMAND"}
		set_tracy_includes()
	end
	if IS_LOGGING_ENABLED == true then
		defines{"LOGGING_ENABLE"}
	end
	if IS_ECS_DEBUG_INFO_ENABLED == true then
		defines{"ECS_DEBUG_INFO_ENABLE=1"}
	else
		defines{"ECS_DEBUG_INFO_ENABLE=0"}
	end

	filter{"configurations:debug"}
		defines{"DEBUG=1", "BX_CONFIG_DEBUG=1"}
	filter{"configurations:release"}
		defines{"NDEBUG", "RELEASE=1", "BX_CONFIG_DEBUG=0"}
	filter{"configurations:hybrid"}
		defines{"NDEBUG", "HYBRID=1", "BX_CONFIG_DEBUG=0"}
	filter{}
	defines{"_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
			"_CRT_SECURE_NO_WARNINGS",
			"__STDC_FORMAT_MACROS",
			"_CRT_SECURE_NO_DEPRECATE",
			"RTTR_DLL",						-- RTTR
			"USE_MALLOC_LOCK",				-- DLMALLOC concurrency safety
			}

	if IS_WARNING_AND_ERRORS_ENABLED == false then
		set_and_disable_common_warnings_errors()
	end
end

------------------------------------------------------------------------------------------------------------------------
function set_basic_links()
	if PLATFORM == "windows" then
		links{"gdi32", "ws2_32", "kernel32", "opengl32", "psapi", "winmm"}
	elseif PLATFORM == "linux" then
		links{"GL", "rt", "m", "X11", "fcpp", "glslang", "glsl-optimizer", "spirv-opt", "spirv-cross"}
	elseif PLATFORM == "macosx" then
		-- setting macos built-in libraries is required to be done through linkoptions
		linkoptions{"-framework Cocoa -framework CoreVideo -framework CoreFoundation -framework IOKit -framework OpenGL"}
	else
	end
end

------------------------------------------------------------------------------------------------------------------------
function configure()
	filter{"configurations:debug"}
		symbols "On"
		optimize "Off"
	filter{"configurations:hybrid"}
		symbols "On"
		optimize "Full"
	filter{"configurations:release"}
		symbols "Off"
		optimize "Full"
	filter{}
end

------------------------------------------------------------------------------------------------------------------------
function load_project(project, dir)
	include(path.join(dir, project) .. ".lua")
end

------------------------------------------------------------------------------------------------------------------------
function set_include_path_to_engine()
	engine_path = path.join(WORKSPACE_DIR, "engine", "src")

	print("\t\ttincludes engine at '" .. engine_path .. "'")

	externalincludedirs{engine_path}
	includedirs{engine_path}
	files{path.join(engine_path, "**.cpp"),
		  path.join(engine_path, "**.hpp"),
		  path.join(engine_path, "**.h"),
		  path.join(engine_path, "**.c")}
end

------------------------------------------------------------------------------------------------------------------------
function set_include_path_to_self(name)
	externalincludedirs {"include"}
	-- for some thirdparty libraries required, so we set just in case for all
	externalincludedirs {path.join("include", name)}

	includedirs {"include"}
	-- for some thirdparty libraries required, so we set just in case for all
	includedirs {path.join("include", name)}
end

-- Special function for plugins. Sets include directories of a plugins 3rdparty for a plugin
------------------------------------------------------------------------------------------------------------------------
local function set_plugin_thirdparty_include(plugin_name, thirdparty_name)
	local plugin_dir = path.join(WORKSPACE_DIR, PLUGINS_DIR, plugin_name)
	local plugin_thirdparty_dir = path.join(plugin_dir, PLUGINS_THIRDPARTY_DIR, thirdparty_name)

	externalincludedirs { path.join(plugin_thirdparty_dir, "include") }
	includedirs { path.join(plugin_thirdparty_dir, "include") }

	if VERBOSE == true then
		print("\t\tplugin thirdparty include: " .. path.join(plugin_thirdparty_dir, "include"))
	end
end

------------------------------------------------------------------------------------------------------------------------
function set_include_path(type_name, name)

	if string.find(type_name, THIRDPARTY_DIR) then
		externalincludedirs {path.join(WORKSPACE_DIR, THIRDPARTY_DIR, name, "include")}
	elseif string.find(type_name, PLUGINS_DIR) then
		externalincludedirs {path.join(WORKSPACE_DIR, PLUGINS_DIR, name, "src")}
	elseif string.find(type_name, PROJECTS_DIR) then
		externalincludedirs {path.join(WORKSPACE_DIR, PROJECTS_DIR, name, "include")}
	else
		if VERBOSE == true then
			print("Unrecognized type name for include path '" .. name .. "'. Allowed values are '" .. THIRDPARTY_DIR .."', '" .. PLUGINS_DIR .."' and '" .. PROJECTS_DIR .."'")
		end
	end

	if VERBOSE == true then
		if string.find(type_name, THIRDPARTY_DIR) then
			print("\t\tinclude: " .. path.join(WORKSPACE_DIR, THIRDPARTY_DIR, name, "include"))
		elseif string.find(type_name, PLUGINS_DIR) then
			print("\t\tinclude: " .. path.join(WORKSPACE_DIR, PLUGINS_DIR, name, "src"))
		elseif string.find(type_name, PROJECTS_DIR) then
			print("\t\tinclude: " .. path.join(WORKSPACE_DIR, PROJECTS_DIR, name, "include"))
		end
	end
end

------------------------------------------------------------------------------------------------------------------------
function set_bgfx_3rd_party_includes()

	bgfx_dir = path.join(WORKSPACE_DIR, "3rdparty", "bgfx", "bgfx")
	bgfx_3rd_party_dir = path.join(bgfx_dir, "3rdparty")

	externalincludedirs{bgfx_3rd_party_dir,
						path.join(bgfx_3rd_party_dir, "directx-headers/include"),
						path.join(bgfx_3rd_party_dir, "directx-headers/include/directx"),
						path.join(bgfx_3rd_party_dir, "directx-headers/include/wsl"),
						path.join(bgfx_3rd_party_dir, "dxsdk/include"),

						path.join(bgfx_3rd_party_dir, "glsl-optimizer"),
						path.join(bgfx_3rd_party_dir, "glsl-optimizer/include"),
						path.join(bgfx_3rd_party_dir, "glsl-optimizer/src/glsl"),

						path.join(bgfx_3rd_party_dir, "glslang"),
						path.join(bgfx_3rd_party_dir, "glslang/glslang/Public"),
						path.join(bgfx_3rd_party_dir, "glslang/glslang/Include"),
						
						path.join(bgfx_3rd_party_dir, "fcpp"),

						path.join(bgfx_3rd_party_dir, "spirv-cross"),
						path.join(bgfx_3rd_party_dir, "spirv-cross/include"),
						path.join(bgfx_3rd_party_dir, "spirv-headers/include"),
						path.join(bgfx_3rd_party_dir, "spirv-tools/include"),
	}

	includedirs{bgfx_3rd_party_dir,
				path.join(bgfx_3rd_party_dir, "directx-headers/include"),
				path.join(bgfx_3rd_party_dir, "directx-headers/include/directx"),
				path.join(bgfx_3rd_party_dir, "directx-headers/include/wsl"),
				
				path.join(bgfx_3rd_party_dir, "glsl-optimizer"),
				path.join(bgfx_3rd_party_dir, "glsl-optimizer/include"),

				path.join(bgfx_3rd_party_dir, "glslang"),

				path.join(bgfx_3rd_party_dir, "spirv-cross"),
				path.join(bgfx_3rd_party_dir, "spirv-cross/include"),
				path.join(bgfx_3rd_party_dir, "spirv-headers/include"),
				path.join(bgfx_3rd_party_dir, "spirv-tools/include"),
	}

	-- some things required includes to files contained in src directory
	externalincludedirs{path.join(WORKSPACE_DIR, "3rdparty", "bgfx", "bgfx"),
						path.join(WORKSPACE_DIR, "3rdparty", "bgfx", "bgfx", "include")}
end

------------------------------------------------------------------------------------------------------------------------
function set_glfw_deps()
	externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "glfw", "include")}
	links{"glfw"}
end

------------------------------------------------------------------------------------------------------------------------
function set_bgfx_deps()
	externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "fcpp", "include")}
	links{"fcpp"}
end

------------------------------------------------------------------------------------------------------------------------
function set_bx_includes()
	externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "bx", "bx", "include")}
	if PLATFORM == "windows" then
		externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "bx", "bx", "include/compat/msvc")}
	elseif PLATFORM == "linux" then
		externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "bx", "bx", "include/compat/linux")}
	elseif PLATFORM == "macosx" then
		externalincludedirs {path.join(WORKSPACE_DIR, "3rdparty", "bx", "bx", "include/compat/osx")}
	else
		print("Unknown platform!")
	end

	set_glfw_deps()
	set_bgfx_deps()
	set_bgfx_3rd_party_includes()
end

------------------------------------------------------------------------------------------------------------------------
function set_libs_path()
	libdirs{path.join(VENDOR_DIR, OUTDIR)}
end

------------------------------------------------------------------------------------------------------------------------
function set_tracy_includes()
	set_include_path(THIRDPARTY_DIR, "tracy")
	externalincludedirs {path.join(WORKSPACE_DIR, THIRDPARTY_DIR, "tracy", "tracy", "public")}
end

-- Creates a static library project. By default c sources are compiled too.
------------------------------------------------------------------------------------------------------------------------
function add_target_static_library(name, build_options, define_flags, plugin_deps, thirdparty_deps, target_language,
	plugin_headeronly_deps, thirdparty_headeronly_deps, additional_includes, depends_on_engine, depends_on_bgfx)
	if VERBOSE == true then
		print("\tstatic library: " .. name)
	end
	project(name)
		language (target_language)
		location (path.join(".project", name))
		targetname (name)

		kind ("StaticLib")

		files{"src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",
			  "src/**.c"}

		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		externalincludedirs{additional_includes}
		includedirs{additional_includes}
		set_include_path_to_self(name)
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_libs_path()

		-- include and link deps from other plugins and thirdparty
		for ii = 1, #plugin_deps do
			p = plugin_deps[ii]
			links{p}
			set_include_path(PLUGINS_DIR, p)
		end
		for ii = 1, #thirdparty_deps do
			p = thirdparty_deps[ii]
			links{p}
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- set includes only from other plugins and thirdparty
		for ii = 1, #plugin_headeronly_deps do
			p = plugin_headeronly_deps[ii]
			set_include_path(PLUGINS_DIR, p)
		end
		for ii = 1, #thirdparty_headeronly_deps do
			p = thirdparty_headeronly_deps[ii]
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- link with engine by default, if not explicitly disabled
		if depends_on_bgfx == true or depends_on_engine == true then
			set_bx_includes()
		end


		configure()
end

------------------------------------------------------------------------------------------------------------------------
function add_target_library(name, build_options, define_flags, thirdparty_headeronly_deps, thirdparty_deps, headeronly, target_language, additional_includes)
	if VERBOSE == true then
		print("\tshared library: " .. name)
	end
	project(name)
		language (target_language)
		location (path.join(".project", name))

		if headeronly == true then
			kind ("None")
		else
			kind ("SharedLib")
		end

		files{"src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",
			  "src/**.c"}

		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		set_include_path_to_self(name)
		includedirs{additional_includes}
		externalincludedirs{additional_includes}
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_libs_path()

		-- include and link deps from other plugins and thirdparty
		for ii = 1, #thirdparty_headeronly_deps do
			p = thirdparty_headeronly_deps[ii]
			set_include_path(THIRDPARTY_DIR, p)
		end
		for ii = 1, #thirdparty_deps do
			p = thirdparty_deps[ii]
			links{p}
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- set additional default defines
		defines{name .. "_EXPORTS"}

		configure()
end

------------------------------------------------------------------------------------------------------------------------
function add_target_library_ex(name, build_options, define_flags, plugin_deps, thirdparty_deps, headeronly, target_language, additional_includes)
	if VERBOSE == true then
		print("\tshared library: " .. name)
	end
	project(name)
		language (target_language)
		location (path.join(".project", name))

		if headeronly == true then
			kind ("None")
		else
			kind ("SharedLib")
		end

		files{"src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",
			  "src/**.c"}

		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		externalincludedirs{additional_includes}
		includedirs{additional_includes}
		set_include_path_to_self(name)
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		--set_libs_path()

		-- include and link deps from other plugins and thirdparty
		for ii = 1, #plugin_deps do
			p = plugin_deps[ii]
			links{p}
			set_include_path(PLUGINS_DIR, p)
		end
		for ii = 1, #thirdparty_deps do
			p = thirdparty_deps[ii]
			links{p}
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- set additional default defines
		defines{name .. "_EXPORTS"}

		configure()
end

-- Create a plugin static library that is linked into the application. Engine headers will be available inside plugin
------------------------------------------------------------------------------------------------------------------------
function add_target_plugin(name, build_options, define_flags, deps, headeronly_deps, additional_includes)
	if VERBOSE == true then
		print("\tplugin: " .. name)
	end

	-- add dependencies of plugin to global table for application later
	tables_concat(PLUGINS_THIRDPARTY_LIBRARIES, deps)
	tables_concat(PLUGINS_THRIDPARTY_HEADERONLY, headeronly_deps)
	tables_concat(PLUGINS_ADDITIONAL_INCLUDES, additional_includes)

	project(name)
		language ("c++")
		location (path.join(".project", name))

		kind ("StaticLib")

		files{"src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",
			  "src/**.c"}

		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		externalincludedirs {additional_includes}
		externalincludedirs {ENGINE_ADDITIONAL_INCLUDES}
		externalincludedirs {"src"}
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_libs_path()

		-- add engine source and includes
		externalincludedirs {path.join(WORKSPACE_DIR, "engine", "src")}
		set_bx_includes()

		-- add engines dependencies as includes without linking to them
		for ii = 1, #ENGINE_THIRDPARTY_LIBRARIES do
			p = ENGINE_THIRDPARTY_LIBRARIES[ii]
			set_include_path(THIRDPARTY_DIR, p)
		end
		for ii = 1, #ENGINE_THIRDPARTY_HEADERONLY do
			p = ENGINE_THIRDPARTY_HEADERONLY[ii]
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- include and link to plugins dependencies
		for ii = 1, #deps do
			p = deps[ii]
			links{p}
			set_plugin_thirdparty_include(name, p)
		end
		for ii = 1, #headeronly_deps do
			p = headeronly_deps[ii]
			set_plugin_thirdparty_include(name, p)
		end

		defines{name.. "_EXPORTS"}

		configure()
end

-- Create an application for given project to serve only as entry point and executable. Note: mostly a copy of add_target_app
-- but does not include ALL projects source files, only two specific ones.
------------------------------------------------------------------------------------------------------------------------
function add_project_main_app(name, build_options, define_flags, thirdparty_deps, thirdparty_headeronly_deps, additional_includes)

	app_name = name

	if VERBOSE == true then
		print("\tproject main application: " .. name .. " (".. app_name ..")")
	end

	project(app_name)
		location (path.join(".project", app_name))

		kind ("WindowedApp")
		files{"main.cpp",
			  "src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",
			  "src/**.c"}

		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		externalincludedirs{"include"}
		externalincludedirs{additional_includes}
		includedirs{additional_includes}
		externalincludedirs{"src"}
		includedirs{"src"}
		set_include_path_to_engine()
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_basic_links()
		set_bx_includes()
		set_libs_path()
		set_engine_resources_dir()
		set_project_resources_dir()

		-- include and link thirdparty deps
		for ii = 1, #thirdparty_deps do
			p = thirdparty_deps[ii]
			links{p}
			set_include_path(THIRDPARTY_DIR, p)
		end
		for ii = 1, #thirdparty_headeronly_deps do
			p = thirdparty_headeronly_deps[ii]
			set_include_path(THIRDPARTY_DIR, p)
		end

		-- link plugin libraries and their dependencies to main application
		-- Note: as of now we do not use PLUGINS_THRIDPARTY_HEADERONLY and PLUGINS_ADDITIONAL_INCLUDES
		-- because we do not need the headers in main application.
		for ii = 1, #PLUGINS do
			p = PLUGINS[ii]
			links{p}

			-- Note: we link plugin as whole to main application, this is required for
			-- correct functioning of RTTR registration
			if PLATFORM == "windows" then
				linkoptions { "/WHOLEARCHIVE:" .. p .. ".lib" }
			elseif PLATFORM == "linux" or PLATFORM == "macosx" then
				-- linux and macosx are expecting a full library path and not the name only
				local full_plugin_path = path.join(VENDOR_DIR, OUTDIR, "lib" .. p .. ".a")

				if PLATFORM == "linux" then
					linkoptions { "-Wl,--whole-archive", full_plugin_path, "-Wl,--no-whole-archive" }
				elseif PLATFORM == "macosx" then
					linkoptions { "-Wl,-force_load," .. full_plugin_path }
				end
			end
		end

		for ii = 1, #PLUGINS_THIRDPARTY_LIBRARIES do
			p = PLUGINS_THIRDPARTY_LIBRARIES[ii]
			links{p}
		end

		configure()

		if IS_TRACY_ENABLED == true and IS_PROFILE_ENABLED == true then
			-- Add single tracy client file to project
			files{ path.join(WORKSPACE_DIR, THIRDPARTY_DIR, "tracy", "tracy", "public", "TracyClient.cpp") }
		end
end

-- Create a project, note that projects depend on the engine and each plugin and thirdparty the engine depends on.
------------------------------------------------------------------------------------------------------------------------
function add_target_project(name, build_options, define_flags, thirdparty_deps, thirdparty_headeronly_deps, additional_includes)

	group("projects/".. name)
		add_project_main_app(name, build_options, define_flags, thirdparty_deps, thirdparty_headeronly_deps, additional_includes)
	group ""
end
