include (path.join(SCRIPTS_DIR, "utility.lua"))

function box2d()
	name = "box2d"
	build_options = {}
	define_flags = {"box2d_EXPORTS", "BOX2D_EXPORT"}
	additional_includes = {"box2d/include", "box2d/src"}
	box2d_path = "box2d/src"

	project(name)
		language ("c++")
		location (path.join(".project", name))
		targetname (name)

		kind ("SharedLib")

		files{"src/**.h",
			  "src/**.cpp",
			  "src/**.hpp",

				path.join("include/box2d.h"),
				path.join(box2d_path, "aabb.c"),
				path.join(box2d_path, "aabb.h"),
				path.join(box2d_path, "arena_allocator.c"),
				path.join(box2d_path, "arena_allocator.h"),
				path.join(box2d_path, "array.c"),
				path.join(box2d_path, "array.h"),
				path.join(box2d_path, "atomic.h"),
				path.join(box2d_path, "bitset.c"),
				path.join(box2d_path, "bitset.h"),
				path.join(box2d_path, "body.c"),
				path.join(box2d_path, "body.h"),
				path.join(box2d_path, "broad_phase.c"),
				path.join(box2d_path, "broad_phase.h"),
				path.join(box2d_path, "constants.h"),
				path.join(box2d_path, "constraint_graph.c"),
				path.join(box2d_path, "constraint_graph.h"),
				path.join(box2d_path, "contact.c"),
				path.join(box2d_path, "contact.h"),
				path.join(box2d_path, "contact_solver.c"),
				path.join(box2d_path, "contact_solver.h"),
				path.join(box2d_path, "core.c"),
				path.join(box2d_path, "core.h"),
				path.join(box2d_path, "ctz.h"),
				path.join(box2d_path, "distance.c"),
				path.join(box2d_path, "distance_joint.c"),
				path.join(box2d_path, "dynamic_tree.c"),
				path.join(box2d_path, "geometry.c"),
				path.join(box2d_path, "hull.c"),
				path.join(box2d_path, "id_pool.c"),
				path.join(box2d_path, "id_pool.h"),
				path.join(box2d_path, "island.c"),
				path.join(box2d_path, "island.h"),
				path.join(box2d_path, "joint.c"),
				path.join(box2d_path, "joint.h"),
				path.join(box2d_path, "manifold.c"),
				path.join(box2d_path, "math_functions.c"),
				path.join(box2d_path, "motor_joint.c"),
				path.join(box2d_path, "mouse_joint.c"),
				path.join(box2d_path, "prismatic_joint.c"),
				path.join(box2d_path, "revolute_joint.c"),
				path.join(box2d_path, "sensor.c"),
				path.join(box2d_path, "sensor.h"),
				path.join(box2d_path, "shape.c"),
				path.join(box2d_path, "shape.h"),
				path.join(box2d_path, "solver.c"),
				path.join(box2d_path, "solver.h"),
				path.join(box2d_path, "solver_set.c"),
				path.join(box2d_path, "solver_set.h"),
				path.join(box2d_path, "table.c"),
				path.join(box2d_path, "table.h"),
				path.join(box2d_path, "timer.c"),
				path.join(box2d_path, "types.c"),
				path.join(box2d_path, "weld_joint.c"),
				path.join(box2d_path, "wheel_joint.c"),
				path.join(box2d_path, "world.c"),
				path.join(box2d_path, "world.h"),
			}

		externalincludedirs {additional_includes}
		includedirs {additional_includes}
		buildoptions{build_options}
		set_basic_defines()
		defines{define_flags}
		set_include_path_to_self(name)
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_libs_path()
		configure()
		
		if PLATFORM == "windows" then
			defines {"_Static_assert=static_assert"}
		end
end
box2d()
