include (path.join(SCRIPTS_DIR, "utility.lua"))

function samples()
	-- Create the project with main entry point application
		add_target_project("samples",
		{}, --build options
		{}, -- define flags
		ENGINE_THIRDPARTY_LIBRARIES,
		ENGINE_THIRDPARTY_HEADERONLY,
		ENGINE_ADDITIONAL_INCLUDES)
end
samples()