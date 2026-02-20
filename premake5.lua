include "scripts/utility.lua"

VERBOSE			= true
SCRIPTS_DIR		= "<undefined>"
VENDOR_DIR		= "<undefined>"
WORKSPACE_DIR	= "<undefined>"
THIRDPARTY_DIR	= "3rdparty"
PLUGINS_DIR		= "plugins"
PROJECTS_DIR	= "projects"
PLUGINS_THIRDPARTY_DIR = "3rdparty" -- Name of 3rdparty folder inside an individual plugin folder
THIRDPARTY = {"bgfx",
			  "bimg",
			  "box2d",
			  "bx",
			  "fcpp",
			  "flecs",
			  "glfw",
			  "nlohmann",
			  "rttr"
			  }

ENGINE_THIRDPARTY_LIBRARIES 	= {"flecs", "rttr", "bgfx", "bimg", "bx", "box2d", "glfw"}
ENGINE_THIRDPARTY_HEADERONLY 	= {"nlohmann"}
ENGINE_ADDITIONAL_INCLUDES 		= {"../../3rdparty/rttr/rttr/src", "../../3rdparty/asio/include/asio/asio/include"}
PLUGINS_THIRDPARTY_LIBRARIES 	= {}
PLUGINS_THRIDPARTY_HEADERONLY 	= {}
PLUGINS_ADDITIONAL_INCLUDES 	= {}
PLUGINS 						= {}

-- add your project to projects and set it as active
PROJECTS = {"samples"}
ACTIVE_PROJECT = "samples"

PLATFORM		= "<undefined>"
OUTDIR			= "%{cfg.buildcfg}-%{cfg.system}"
SYSTEM_VERSION	= "<undefined>"

-- determine available platforms
if os.host() == "linux" then
	SYSTEM_VERSION = "latest"
	PLATFORM = "linux"
	system "linux"
elseif os.host() == "windows" then
	SYSTEM_VERSION = "latest"
	PLATFORM = "windows"
	system "windows"
elseif os.host() == "macosx" then
	SYSTEM_VERSION = "10.15"
	PLATFORM = "macosx"
	system "macosx"
else
	print("Unrecognoized or unsupported platform: " .. os.host())
	return
end

if PLATFORM == "linux" or PLATFORM == "macosx" then
	local libs = {
	"fcpp", "glslang", "glsl-optimizer", "spirv-opt", "spirv-cross"}

	for _, lib in ipairs(libs) do
		table.insert(ENGINE_THIRDPARTY_LIBRARIES, lib)
	end
end


workspace("Workspace")
	startproject(ACTIVE_PROJECT)
	architecture "x86_64"
	configurations{"debug", "hybrid", "release"}
	language "C++"
	cppdialect "Default"
	systemversion(SYSTEM_VERSION)
	staticruntime "Off"
	flags{"MultiProcessorCompile"}

	-- setup variables
	WORKSPACE_DIR = os.getcwd()
	VENDOR_DIR = path.join(WORKSPACE_DIR, "vendor")
	SCRIPTS_DIR = path.join(WORKSPACE_DIR, "scripts")

	-- create 3rdparty dependency projects
	print("Loading 3rdparty projects...")
	group "3rdparty"
		for ii = 1, #THIRDPARTY do
			p = THIRDPARTY[ii]

			load_project(p, path.join("3rdparty", p))
		end
	group ""

	-- create plugin projects
	print("Loading plugin projects...")
	group "plugins"
		for ii = 1, #PLUGINS do
			p = PLUGINS[ii]

			group("plugins/" .. p)
			load_project(p, path.join("plugins", p))
			group ""
		end
	group ""

	-- create projects
	print("Loading projects...")
	group "projects"
		for ii = 1, #PROJECTS do
			p = PROJECTS[ii]

			load_project(p, path.join("projects", p))
		end
	group""
