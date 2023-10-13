#pragma once
#include "log.hpp"

#include <stdio.h>
#include <debugbreak.h>

#ifdef _DEBUG
    #define ASSERT(condition, tag, msg) do { \
        if (!(condition)) { \
            LOG_ERROR(tag, (msg));\
            debug_break(); \
        } \
    } while(0);
#else
    #define ASSERT(condition, tag, msg) 
#endif