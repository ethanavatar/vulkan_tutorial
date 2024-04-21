cmake_minimum_required(VERSION 3.26.4)

project(vulkan_tutorial C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(vulkan_tutorial vulkan_tutorial.c)

add_subdirectory(glfw)
add_subdirectory(cglm)

target_include_directories(glfw PRIVATE $ENV{VULKAN_SDK}/Include)

target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/glfw/include)
target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/cglm/include)
target_include_directories(vulkan_tutorial PRIVATE $ENV{VULKAN_SDK}/Include)

target_link_libraries(vulkan_tutorial $ENV{VULKAN_SDK}/Lib/vulkan-1.lib)
target_link_libraries(vulkan_tutorial glfw cglm)
