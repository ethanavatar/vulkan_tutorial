cmake_minimum_required(VERSION 3.24.0)

project(vulkan_tutorial C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_VULKAN_STATIC ON)
set(GLFW_USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
add_subdirectory(glfw)

#set(CGLM_STATIC ON)
#set(CGLM_SHARED OFF)
#add_subdirectory(cglm)

# MSVC
if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
    message("MSVC")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

# Clang
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
    message("Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
    set(CMAKE_C_COMPILER_TARGET x86_64-w64-windows-gnu)
endif ()

set(CMAKE_BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(vulkan_tutorial vulkan_tutorial.c debug_messenger.c extensions.c shader_modules.c swap_chain.c)

target_include_directories(glfw PRIVATE $ENV{VULKAN_SDK}/Include)

target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/glfw/include)
#target_include_directories(vulkan_tutorial PRIVATE ${CMAKE_SOURCE_DIR}/cglm/include)
target_include_directories(vulkan_tutorial PRIVATE $ENV{VULKAN_SDK}/Include)

target_link_libraries(vulkan_tutorial $ENV{VULKAN_SDK}/Lib/vulkan-1.lib)
#target_link_libraries(vulkan_tutorial $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib)
target_link_libraries(vulkan_tutorial
    glfw
    #cglm
)
