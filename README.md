# Vulkan Tutorial

My personal following of [Vulkan Tutorial](https://vulkan-tutorial.com/) in C rather than C++.

## Notes for myself

```nu
# For MSVC
> cmake -S . -B msvc_build
> cmake --build msvc_build --config Release; .\msvc_build\Release\vulkan_tutorial.exe

# For Clang on Windows
> cmake -S . -B clang_build -G "Ninja Multi-Config" $'-DCMAKE_C_COMPILER=(which clang | get 0 | get path | str replace '\\' '/' --all --regex)'
> cmake --build clang_build\ --config Release; .\clang_build\Release\vulkan_tutorial.exe
```

```nu
# Compiling shaders
# TODO: Make an option to do this at runtime with shaderc
> glslc shaders/shader.vert -o shaders/vert.spv
> glslc shaders/shader.frag -o shaders/frag.spv
```
