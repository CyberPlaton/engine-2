include (path.join(SCRIPTS_DIR, "utility.lua"))

function dlmalloc()
	add_target_library("dlmalloc",
					{},
					{},
					{},
					{},
					true,
					"c++")
end
dlmalloc()