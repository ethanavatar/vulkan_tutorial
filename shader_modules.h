#ifndef SHADER_MODULES_H
#define SHADER_MODULES_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

/*
enum shader_type {
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
};

VkResult createShaderModule(
	VkDevice device,
	const char *glsl_filename,
	VkShaderModule *shader_module
);
*/

#endif // SHADER_MODULES_H

#ifdef SHADER_MODULES_IMPLEMENTATION

#ifndef FILE_IO_IMPLEMENTATION
#define FILE_IO_IMPLEMENTATION
#endif // FILE_IO_IMPLEMENTATION

#include "file_io.h"

// TODO: Runtime compilation of GLSL to SPIR-V
// This one is broken right now
/*
VkResult createShaderModule(
	VkDevice device,
	const char *glsl_filename,
	enum shader_type type,
	VkShaderModule *shader_module
) {
 	const char *source = read_file_bytes(glsl_filename);
 	if (!source) return VK_ERROR_INITIALIZATION_FAILED;

	shaderc_compiler_t compiler = shaderc_compiler_initialize();
 	shaderc_compile_options_t options = shaderc_compile_options_initialize();

	shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);

	shaderc_compile_options_set_target_env(
		options,
		shaderc_target_env_vulkan,
		shaderc_env_version_vulkan_1_0
	);

	shaderc_compile_options_set_source_language(
		options,
		shaderc_source_language_glsl
	);

	shaderc_compile_options_set_auto_map_locations(options, true);
	shaderc_compile_options_set_auto_bind_uniforms(options, true);

	shaderc_shader_kind kind = 
		type == SHADER_TYPE_VERTEX 
		? shaderc_glsl_vertex_shader 
		: shaderc_glsl_fragment_shader;

	shaderc_compilation_result_t result = shaderc_compile_into_spv(
		compiler,
		source,
		strlen(source),
		kind,
		glsl_filename,
		"main",
		options
	);

	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		fprintf(stderr, "Error compiling shader %s\n", glsl_filename);
		shaderc_result_release(result);
		free((void *)source);
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	size_t size = shaderc_result_get_length(result);
	const char *data = shaderc_result_get_bytes(result);

	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (const uint32_t *)data
	};

	VkResult vk_result = vkCreateShaderModule(device, &create_info, NULL, shader_module);

	shaderc_result_release(result);
	free((void *) source);

	return vk_result;
}
*/


#endif // SHADER_MODULES_IMPLEMENTATION
