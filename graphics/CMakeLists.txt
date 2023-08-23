cmake_minimum_required(VERSION 3.0)

add_library(graphics GraphicsContext.cpp Window.cpp Device.cpp GPUAllocator.cpp Swapchain.cpp)

target_include_directories(graphics PUBLIC ${CMAKE_SOURCE_DIR})

find_package(Vulkan REQUIRED)
target_link_libraries(graphics PRIVATE Vulkan::Vulkan)

if (UNIX)
  find_package(glfw3 REQUIRED)
endif()
target_link_libraries(graphics PRIVATE glfw)
