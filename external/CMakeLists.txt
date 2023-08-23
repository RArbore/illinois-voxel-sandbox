cmake_minimum_required(VERSION 3.0)

find_package(Vulkan REQUIRED)

if (WIN32)
  add_subdirectory(GLFW)
endif()

add_library(vma VmaImpl.cpp)

find_package(Vulkan REQUIRED)
target_link_libraries(vma PRIVATE Vulkan::Vulkan)
