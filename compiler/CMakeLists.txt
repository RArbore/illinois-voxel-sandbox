cmake_minimum_required(VERSION 3.0)

set (COMPILER_SRC
  Compiler.cpp
)

add_library(compiler ${COMPILER_SRC})

target_include_directories(compiler PUBLIC ${CMAKE_SOURCE_DIR})
