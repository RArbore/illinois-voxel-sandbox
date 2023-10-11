cmake_minimum_required(VERSION 3.0)

set(GRAPHICS_SRC
  GraphicsContext.cpp
  Camera.cpp
  Window.cpp
  Device.cpp
  GPUAllocator.cpp
  Swapchain.cpp
  Command.cpp
  Pipeline.cpp
  Synchronization.cpp
  Descriptor.cpp
  RingBuffer.cpp
  Geometry.cpp
)

add_library(graphics ${GRAPHICS_SRC})

target_include_directories(graphics PUBLIC ${CMAKE_SOURCE_DIR})

find_package(Vulkan REQUIRED)
target_link_libraries(graphics PUBLIC Vulkan::Vulkan)

if (UNIX)
    find_package(glfw3 REQUIRED)
endif ()
target_link_libraries(graphics PUBLIC glfw)
