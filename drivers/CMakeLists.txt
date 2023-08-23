cmake_minimum_required(VERSION 3.0)

add_executable(ivs ivs.cpp)

target_include_directories(ivs PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(ivs LINK_PUBLIC graphics)
target_link_libraries(ivs LINK_PUBLIC utils)
add_dependencies(ivs shaders)

target_link_libraries(ivs LINK_PUBLIC vma)

find_package(Vulkan REQUIRED)
target_link_libraries(ivs PRIVATE Vulkan::Vulkan)

if (UNIX)
  find_package(glfw3 REQUIRED)
endif()
target_link_libraries(ivs PRIVATE glfw)
