include (path.join(SCRIPTS_DIR, "utility.lua"))

function fmt()
	add_target_library("fmt",
					{},
					{},
					{},
					{},
					true,
					"c++")
end
fmt()