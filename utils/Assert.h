#include <iostream>

#include <vulkan/vulkan.h>

void assert_impl(VkResult result, const std::string_view msg, const std::string_view file, uint32_t line);

#define ASSERT(res, msg)			\
    assert_impl(res, msg, __FILE__, __LINE__);
