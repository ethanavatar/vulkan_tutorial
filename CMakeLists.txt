cmake_minimum_required(VERSION 3.24.0)

project(vulkan_tutorial C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic")
endif()

set(CMAKE_BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(vulkan_tutorial vulkan_tutorial.c debug_messenger.c vulkan_extensions.c)

add_subdirectory(glfw)

set(CGLM_STATIC ON)
set(CGLM_SHARED OFF)
add_subdirectory(cglm)

target_include_directories(glfw PRIVATE $ENV{VULKAN_SDK}/Include)

target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/glfw/include)
target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/cglm/include)
target_include_directories(vulkan_tutorial PRIVATE $ENV{VULKAN_SDK}/Include)

target_link_libraries(vulkan_tutorial $ENV{VULKAN_SDK}/Lib/vulkan-1.lib)
target_link_libraries(vulkan_tutorial $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib)
target_link_libraries(vulkan_tutorial glfw cglm)
