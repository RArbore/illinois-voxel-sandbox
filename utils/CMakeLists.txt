cmake_minimum_required(VERSION 3.0)

add_library(utils Assert.cpp)

find_package(Vulkan REQUIRED)
target_link_libraries(utils PRIVATE Vulkan::Vulkan)
