#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #if defined(_DEBUG)
        #define IG_QT_MODULE_EXPORT
    #elif defined(IG_QT_COMPILE_EXPORT)
        #define IG_QT_MODULE_EXPORT __declspec(dllexport)
    #elif !defined(IG_QT_STATIC)
        // 动态库消费者走 dllimport；静态库（IG_QT_STATIC）不使用任何导出宏
        #define IG_QT_MODULE_EXPORT __declspec(dllimport)
    #else
        #define IG_QT_MODULE_EXPORT
#endif
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #if defined(IG_QT_COMPILE_EXPORT)
        #define IG_QT_MODULE_EXPORT __attribute__((visibility("default")))
    #else
        #define IG_QT_MODULE_EXPORT
#endif
#elif defined(EMSCRIPTEN)
    #define IG_QT_MODULE_EXPORT
#endif



