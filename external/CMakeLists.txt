cmake_minimum_required(VERSION 3.0)

find_package(Vulkan REQUIRED)

if (WIN32)
  add_subdirectory(GLFW)
endif()
add_subdirectory(glm)

add_library(vma VmaImpl.cpp)
add_library(ogt OgtImpl.cpp)
add_library(tinyobj TinyOBJImpl.cpp)
add_library(stbimage STBImageImpl.cpp)

add_library(json INTERFACE)
target_sources(json PUBLIC
  FILE_SET HEADERS
    FILES nlohmann_json.hpp
)

find_package(Vulkan REQUIRED)
target_link_libraries(vma PRIVATE Vulkan::Vulkan)
