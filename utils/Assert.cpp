#include <iostream>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

void assert_impl(VkResult result, const std::string_view msg, const std::string_view file, uint32_t line) {
    if (result != VK_SUCCESS) {
	std::cerr << "PANIC: " << msg << "\nFound Vulkan error code: " << string_VkResult(result) << "\nOccurred in " << file << " at line " << line << ".\n";
	__builtin_trap();
	exit(-1);
    }
}
