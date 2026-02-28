include (path.join(SCRIPTS_DIR, "utility.lua"))

function dlmalloc()
	add_target_static_library("dlmalloc",
					{},
					{},
					{},
					{},
					"c++",
					{},
					{},
					{},
					false,
					false)
	files{ "dlmalloc/malloc.c" }
end
dlmalloc()