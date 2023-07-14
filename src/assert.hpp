#pragma once
#include <stdio.h>
#include <debugbreak.h>
#include <spdlog/spdlog.h>

#ifdef _DEBUG
    #define ASSERT(condition, tag, msg) do { \
        if (!(condition)) { \
            spdlog::error("{}: {}\n[file]: {} ({})\n[function]: {}", (tag), &(msg)[0], __FILE__, __LINE__, __FUNCTION__);\
            debug_break(); \
        } \
    } while(0);
#else
    #define ASSERT(condition, tag, msg) 
#endif