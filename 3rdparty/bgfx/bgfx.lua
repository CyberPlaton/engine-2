include (path.join(SCRIPTS_DIR, "utility.lua"))

-- Define projects required for shader compiler when using Vulkan
if PLATFORM == "linux" or PLATFORM == "macosx" then
	function shaderc_projects()
		local BGFX_DIR 		 = path.join(WORKSPACE_DIR, "3rdparty", "bgfx", "bgfx")
		local GLSL_OPTIMIZER = path.join(BGFX_DIR, "3rdparty/glsl-optimizer")
		local GLSLANG        = path.join(BGFX_DIR, "3rdparty/glslang")
		local SPIRV_CROSS    = path.join(BGFX_DIR, "3rdparty/spirv-cross")
		local SPIRV_HEADERS  = path.join(BGFX_DIR, "3rdparty/spirv-headers")
		local SPIRV_TOOLS    = path.join(BGFX_DIR, "3rdparty/spirv-tools")

		-- Spirv-Opt Project
		project "spirv-opt"
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		location (path.join(".project", "spirv-opt"))
	kind "StaticLib"

	includedirs {
		SPIRV_TOOLS,

		path.join(SPIRV_TOOLS, "include"),
		path.join(SPIRV_TOOLS, "include/generated"),
		path.join(SPIRV_TOOLS, "source"),
		path.join(SPIRV_HEADERS, "include"),
	}

	files {
		path.join(SPIRV_TOOLS, "source/opt/**.cpp"),
		path.join(SPIRV_TOOLS, "source/opt/**.h"),
		path.join(SPIRV_TOOLS, "source/reduce/**.cpp"),
		path.join(SPIRV_TOOLS, "source/reduce/**.h"),
		path.join(SPIRV_TOOLS, "source/val/**.cpp"),
		path.join(SPIRV_TOOLS, "source/val/**.h"),

		-- libspirv
		path.join(SPIRV_TOOLS, "source/assembly_grammar.cpp"),
		path.join(SPIRV_TOOLS, "source/assembly_grammar.h"),
		path.join(SPIRV_TOOLS, "source/binary.cpp"),
		path.join(SPIRV_TOOLS, "source/binary.h"),
		path.join(SPIRV_TOOLS, "source/cfa.h"),
		path.join(SPIRV_TOOLS, "source/diagnostic.cpp"),
		path.join(SPIRV_TOOLS, "source/diagnostic.h"),
		path.join(SPIRV_TOOLS, "source/disassemble.cpp"),
		path.join(SPIRV_TOOLS, "source/disassemble.h"),
		path.join(SPIRV_TOOLS, "source/enum_set.h"),
		path.join(SPIRV_TOOLS, "source/enum_string_mapping.cpp"),
		path.join(SPIRV_TOOLS, "source/enum_string_mapping.h"),
		path.join(SPIRV_TOOLS, "source/ext_inst.cpp"),
		path.join(SPIRV_TOOLS, "source/ext_inst.h"),
		path.join(SPIRV_TOOLS, "source/extensions.cpp"),
		path.join(SPIRV_TOOLS, "source/extensions.h"),
		path.join(SPIRV_TOOLS, "source/instruction.h"),
		path.join(SPIRV_TOOLS, "source/latest_version_glsl_std_450_header.h"),
		path.join(SPIRV_TOOLS, "source/latest_version_opencl_std_header.h"),
		path.join(SPIRV_TOOLS, "source/latest_version_spirv_header.h"),
		path.join(SPIRV_TOOLS, "source/libspirv.cpp"),
		path.join(SPIRV_TOOLS, "source/macro.h"),
		path.join(SPIRV_TOOLS, "source/name_mapper.cpp"),
		path.join(SPIRV_TOOLS, "source/name_mapper.h"),
		path.join(SPIRV_TOOLS, "source/opcode.cpp"),
		path.join(SPIRV_TOOLS, "source/opcode.h"),
		path.join(SPIRV_TOOLS, "source/operand.cpp"),
		path.join(SPIRV_TOOLS, "source/operand.h"),
		path.join(SPIRV_TOOLS, "source/parsed_operand.cpp"),
		path.join(SPIRV_TOOLS, "source/parsed_operand.h"),
		path.join(SPIRV_TOOLS, "source/print.cpp"),
		path.join(SPIRV_TOOLS, "source/print.h"),
		path.join(SPIRV_TOOLS, "source/software_version.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_constant.h"),
		path.join(SPIRV_TOOLS, "source/spirv_definition.h"),
		path.join(SPIRV_TOOLS, "source/spirv_endian.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_endian.h"),
		path.join(SPIRV_TOOLS, "source/spirv_optimizer_options.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_reducer_options.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_target_env.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_target_env.h"),
		path.join(SPIRV_TOOLS, "source/spirv_validator_options.cpp"),
		path.join(SPIRV_TOOLS, "source/spirv_validator_options.h"),
		path.join(SPIRV_TOOLS, "source/table.cpp"),
		path.join(SPIRV_TOOLS, "source/table.h"),
		path.join(SPIRV_TOOLS, "source/text.cpp"),
		path.join(SPIRV_TOOLS, "source/text.h"),
		path.join(SPIRV_TOOLS, "source/text_handler.cpp"),
		path.join(SPIRV_TOOLS, "source/text_handler.h"),
		path.join(SPIRV_TOOLS, "source/to_string.cpp"),
		path.join(SPIRV_TOOLS, "source/to_string.h"),
		path.join(SPIRV_TOOLS, "source/util/bit_vector.cpp"),
		path.join(SPIRV_TOOLS, "source/util/bit_vector.h"),
		path.join(SPIRV_TOOLS, "source/util/bitutils.h"),
		path.join(SPIRV_TOOLS, "source/util/hex_float.h"),
		path.join(SPIRV_TOOLS, "source/util/parse_number.cpp"),
		path.join(SPIRV_TOOLS, "source/util/parse_number.h"),
		path.join(SPIRV_TOOLS, "source/util/string_utils.cpp"),
		path.join(SPIRV_TOOLS, "source/util/string_utils.h"),
		path.join(SPIRV_TOOLS, "source/util/timer.h"),
	}

	filter { "mingw* or linux* or osx*" }
		buildoptions {
			"-Wno-switch",
		}

	filter { "mingw* or linux-gcc-*" }
		buildoptions {
			"-Wno-misleading-indentation",
		}

	filter {}

	set_basic_defines()

	-- Spirv-Cross Project
	project "spirv-cross"
	targetdir(path.join(VENDOR_DIR, OUTDIR))
	objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
	location (path.join(".project", "spirv-cross"))
	kind "StaticLib"

	defines {
		"SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS",
	}

	includedirs {
		path.join(SPIRV_CROSS, "include"),
	}

	files {
		path.join(SPIRV_CROSS, "spirv.hpp"),
		path.join(SPIRV_CROSS, "spirv_cfg.cpp"),
		path.join(SPIRV_CROSS, "spirv_cfg.hpp"),
		path.join(SPIRV_CROSS, "spirv_common.hpp"),
		path.join(SPIRV_CROSS, "spirv_cpp.cpp"),
		path.join(SPIRV_CROSS, "spirv_cpp.hpp"),
		path.join(SPIRV_CROSS, "spirv_cross.cpp"),
		path.join(SPIRV_CROSS, "spirv_cross.hpp"),
		path.join(SPIRV_CROSS, "spirv_cross_parsed_ir.cpp"),
		path.join(SPIRV_CROSS, "spirv_cross_parsed_ir.hpp"),
		path.join(SPIRV_CROSS, "spirv_cross_util.cpp"),
		path.join(SPIRV_CROSS, "spirv_cross_util.hpp"),
		path.join(SPIRV_CROSS, "spirv_glsl.cpp"),
		path.join(SPIRV_CROSS, "spirv_glsl.hpp"),
		path.join(SPIRV_CROSS, "spirv_hlsl.cpp"),
		path.join(SPIRV_CROSS, "spirv_hlsl.hpp"),
		path.join(SPIRV_CROSS, "spirv_msl.cpp"),
		path.join(SPIRV_CROSS, "spirv_msl.hpp"),
		path.join(SPIRV_CROSS, "spirv_parser.cpp"),
		path.join(SPIRV_CROSS, "spirv_parser.hpp"),
		path.join(SPIRV_CROSS, "spirv_reflect.cpp"),
		path.join(SPIRV_CROSS, "spirv_reflect.hpp"),
	}

	filter { "mingw* or linux* or osx*" }
		buildoptions {
			"-Wno-type-limits",
		}

	filter {}

	set_basic_defines()

		-- GLSLANG Project
		project "glslang"
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		location (path.join(".project", "spirv-glslang"))
		kind "StaticLib"
	
		defines {
			"ENABLE_OPT=1", -- spirv-tools
			"ENABLE_HLSL=1",
		}
	
		includedirs {
			GLSLANG,
			path.join(GLSLANG, ".."),
			path.join(SPIRV_TOOLS, "include"),
			path.join(SPIRV_TOOLS, "source"),
		}
	
		files {
			path.join(GLSLANG, "glslang/**.cpp"),
			path.join(GLSLANG, "glslang/**.h"),
	
			path.join(GLSLANG, "hlsl/**.cpp"),
			path.join(GLSLANG, "hlsl/**.h"),
	
			path.join(GLSLANG, "SPIRV/**.cpp"),
			path.join(GLSLANG, "SPIRV/**.h"),
	
			path.join(GLSLANG, "OGLCompilersDLL/**.cpp"),
			path.join(GLSLANG, "OGLCompilersDLL/**.h"),
		}
	
		if PLATFORM ~= "windows" then
			removefiles {
				path.join(GLSLANG, "glslang/OSDependent/Windows/**.cpp"),
				path.join(GLSLANG, "glslang/OSDependent/Windows/**.h"),
			}
		end

		filter { "mingw-gcc or linux-gcc" }
			buildoptions {
				"-Wno-logical-op",
				"-Wno-maybe-uninitialized",
			}
	
			filter { "mingw* or linux* or osx*" }
			buildoptions {
				"-fno-strict-aliasing", -- glslang has bugs if strict aliasing is used.
				"-Wno-ignored-qualifiers",
				"-Wno-implicit-fallthrough",
				"-Wno-missing-field-initializers",
				"-Wno-reorder",
				"-Wno-return-type",
				"-Wno-shadow",
				"-Wno-sign-compare",
				"-Wno-switch",
				"-Wno-undef",
				"-Wno-unknown-pragmas",
				"-Wno-unused-function",
				"-Wno-unused-parameter",
				"-Wno-unused-variable",
			}
	
			filter { "osx*" }
			buildoptions {
				"-Wno-c++11-extensions",
				"-Wno-unused-const-variable",
				"-Wno-deprecated-register",
			}
	
			filter { "linux-gcc-*" }
			buildoptions {
				"-Wno-unused-but-set-variable",
			}
	
			filter {}

			set_basic_defines()

		-- GLSL-Optimizer Project
		project "glsl-optimizer"
		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		location (path.join(".project", "glsl-optimizer"))
		kind "StaticLib"
	
		includedirs {
			path.join(GLSL_OPTIMIZER, "src"),
			path.join(GLSL_OPTIMIZER, "include"),
			path.join(GLSL_OPTIMIZER, "src/mesa"),
			path.join(GLSL_OPTIMIZER, "src/mapi"),
			path.join(GLSL_OPTIMIZER, "src/glsl"),
		}
	
		files {
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp-lex.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp-parse.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp-parse.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/pp.c"),
	
			path.join(GLSL_OPTIMIZER, "src/glsl/ast.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ast_array_index.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ast_expr.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ast_function.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ast_to_hir.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ast_type.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_functions.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_type_macros.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_types.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_variables.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_lexer.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_optimizer.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_optimizer.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_parser.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_parser.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_parser_extras.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_parser_extras.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_symbol_table.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_symbol_table.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_types.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glsl_types.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/hir_field_selection.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_basic_block.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_basic_block.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_builder.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_builder.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_clone.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_constant_expression.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_equals.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_expression_flattening.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_expression_flattening.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_function.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_function_can_inline.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_function_detect_recursion.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_function_inlining.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_hierarchical_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_hierarchical_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_hv_accept.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_import_prototypes.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_optimization.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_glsl_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_glsl_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_metal_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_metal_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_print_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_rvalue_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_rvalue_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_stats.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_stats.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_uniform.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_unused_structs.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_unused_structs.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_validate.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_variable_refcount.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_variable_refcount.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_atomics.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_functions.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_interface_blocks.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_uniform_block_active_visitor.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_uniform_block_active_visitor.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_uniform_blocks.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_uniform_initializers.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_uniforms.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_varyings.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/link_varyings.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/linker.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/linker.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/list.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/loop_analysis.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/loop_analysis.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/loop_controls.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/loop_unroll.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_clip_distance.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_discard.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_discard_flow.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_if_to_cond_assign.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_instructions.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_jumps.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_mat_op_to_vec.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_named_interface_blocks.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_noise.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_offset_array.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_output_reads.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_packed_varyings.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_packing_builtins.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_ubo_reference.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_variable_index_to_cond_assign.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_vec_index_to_cond_assign.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_vec_index_to_swizzle.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_vector.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_vector_insert.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/lower_vertex_id.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_algebraic.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_array_splitting.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_constant_folding.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_constant_propagation.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_constant_variable.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_copy_propagation.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_copy_propagation_elements.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_cse.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_dead_builtin_variables.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_dead_builtin_varyings.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_dead_code.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_dead_code_local.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_dead_functions.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_flatten_nested_if_blocks.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_flip_matrices.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_function_inlining.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_if_simplification.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_minmax.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_noop_swizzle.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_rebalance_tree.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_redundant_jumps.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_structure_splitting.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_swizzle_swizzle.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_tree_grafting.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/opt_vectorize.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/program.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/s_expression.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/s_expression.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/standalone_scaffolding.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/standalone_scaffolding.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/strtod.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/strtod.h"),
	
			path.join(GLSL_OPTIMIZER, "src/mesa/main/compiler.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/config.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/context.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/core.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/dd.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/errors.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/glheader.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/glminimal.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/imports.c"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/imports.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/macros.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/mtypes.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/main/simple_list.h"),
	
			path.join(GLSL_OPTIMIZER, "src/mesa/program/hash_table.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/prog_hash_table.c"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/prog_instruction.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/prog_parameter.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/prog_statevars.h"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/symbol_table.c"),
			path.join(GLSL_OPTIMIZER, "src/mesa/program/symbol_table.h"),
	
			path.join(GLSL_OPTIMIZER, "src/util/hash_table.c"),
			path.join(GLSL_OPTIMIZER, "src/util/hash_table.h"),
			path.join(GLSL_OPTIMIZER, "src/util/macros.h"),
			path.join(GLSL_OPTIMIZER, "src/util/ralloc.c"),
			path.join(GLSL_OPTIMIZER, "src/util/ralloc.h"),
		}
	
		removefiles {
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/tests/**"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.l"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.y"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_set_program_inouts.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/main.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_stubs.cpp"),
		}
	
		filter { "Release" }
			optimize "Full"

			filter { "mingw* or linux* or osx*" }
			buildoptions {
				"-fno-strict-aliasing", -- glsl-optimizer has bugs if strict aliasing is used.
	
				"-Wno-implicit-fallthrough",
				"-Wno-parentheses",
				"-Wno-sign-compare",
				"-Wno-unused-function",
				"-Wno-unused-parameter",
			}
	
			removebuildoptions {
				"-Wshadow", -- glsl-optimizer is full of -Wshadow warnings ignore it.
			}
	
			filter { "osx*" }
			buildoptions {
				"-Wno-deprecated-register",
			}
	
			filter { "mingw* or linux-gcc-*" }
			buildoptions {
				"-Wno-misleading-indentation",
			}
	
			filter {}

			set_basic_defines()
	end
	shaderc_projects()
end

function bgfx()
	project("bgfx")
		language ("c++")
		cppdialect "c++17"
		location (path.join(".project", "bgfx"))

		kind ("StaticLib")

		if PLATFORM == "windows" or PLATFORM == "linux" then
			files{path.join("bgfx", "src", "amalgamated.cpp")}
		elseif PLATFORM == "macosx" then
			files{path.join("bgfx", "src", "amalgamated.mm")}
		else
		end

		includedirs{"bgfx/3rdparty",
					"bgfx/include",
					"bgfx/3rdparty",
					"bgfx/3rdparty/cgltf",
					"bgfx/3rdparty/dear-imgui",
					"bgfx/3rdparty/directx-headers/include/directx",
					"bgfx/3rdparty/directx-headers/include/wsl",
					"bgfx/3rdparty/glslang",
					"bgfx/3rdparty/glsl-optimizer",
					"bgfx/3rdparty/glsl-optimizer/include",
					"bgfx/3rdparty/iconfontheaders",
					"bgfx/3rdparty/khronos",
					"bgfx/3rdparty/meshoptimizer",
					"bgfx/3rdparty/native_app_glue",
					"bgfx/3rdparty/renderdoc",
					"bgfx/3rdparty/sdf",
					"bgfx/3rdparty/spirv-cross",
					"bgfx/3rdparty/spirv-cross/include",
					"bgfx/3rdparty/spirv-headers/include",
					"bgfx/3rdparty/spirv-tools/include",
					"bgfx/3rdparty/stv",
					path.join(WORKSPACE_DIR, "3rdparty", "bimg", "bimg", "include")}

		set_bx_includes()

		targetdir(path.join(VENDOR_DIR, OUTDIR))
		objdir(path.join(VENDOR_DIR, OUTDIR, ".obj"))
		set_libs_path()
		set_basic_defines()
		set_basic_links()

		configure()
end
bgfx()