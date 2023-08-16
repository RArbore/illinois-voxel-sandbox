cmake_minimum_required(VERSION 3.0)

add_library(graphics GraphicsContext.cpp Window.cpp Device.cpp)

target_include_directories(graphics PUBLIC ${CMAKE_SOURCE_DIR})
