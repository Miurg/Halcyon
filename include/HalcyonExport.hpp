#pragma once

#ifdef _WIN32
    #ifdef HALCYON_BUILD_SHARED
        #ifdef HALCYON_EXPORTS
            #define HALCYON_API __declspec(dllexport)
        #else
            #define HALCYON_API __declspec(dllimport)
        #endif
    #else
        #define HALCYON_API
    #endif
#else
    #define HALCYON_API
#endif
