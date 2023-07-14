#pragma once
#include <stdio.h>
#include <debugbreak.h>

#ifdef _DEBUG
    #define ASSERT(condition, tag, msg) do { \
        if (!(condition)) { \
            printf_s("[%s]: %s\n[file]: %s (%d)\n[function]: %s\n\n", (tag), &(msg)[0], __FILE__, __LINE__, __FUNCTION__); \
            debug_break(); \
        } \
    } while(0);
#else
    #define ASSERT(condition, tag, msg) 
#endif