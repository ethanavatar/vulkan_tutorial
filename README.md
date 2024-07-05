# Vulkan Tutorial

My personal following of [Vulkan Tutorial](https://vulkan-tutorial.com/) in C rather than C++.

## Notes for myself

```nu
# Nushell

# For MSVC
> cmake -S . -B msvc_build
> cmake --build msvc_build --config Release

# For Clang on Windows
> cmake -S . -B clang_build -G "Ninja Multi-Config" $'-DCMAKE_C_COMPILER=(which clang | get 0 | get path | str replace '\\' '/' --all --regex)'
> cmake --build clang_build --config Release
```
