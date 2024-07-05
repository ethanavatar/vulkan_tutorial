#pragma once
#ifndef GLOBALS_H
#define GLOBALS_H

#include <vulkan/vulkan.h>

#ifdef NDEBUG
#   define ENABLE_VALIDATION_LAYERS false
#else
#   define ENABLE_VALIDATION_LAYERS true
#endif

#define STRINGIFY(x) #x
#define UNUSED_INTENTIONAL(x) { (void) (x); }
#define UNUSED(x) {\
    fprintf(stderr, "UNUSED VARIABLE: %s\n", STRINGIFY(x));\
    (void) (x); }

#define TODO(comment) {\
    fprintf(stderr, "TODO (%s:%d): %s\n", __FILE__, __LINE__, comment); }

#define FIXME(comment) {\
    fprintf(stderr, "FIXME (%s:%d): %s\n", __FILE__, __LINE__, comment); }

#define RETURN_IF_NOT_VK_SUCCESS(R, MSG) do {\
    if (R != VK_SUCCESS) {\
        fprintf(stderr, "Error: %s\n", MSG);\
        return R;\
    }\
} while(0)

#if defined(_MSC_VER)
#   include <malloc.h>
#   define alloca _alloca
#endif

#endif // GLOBALS_H
