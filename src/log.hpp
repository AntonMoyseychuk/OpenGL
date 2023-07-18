#pragma once
#include <spdlog/spdlog.h>

#ifdef _DEBUG
    #define LOG_ERROR(tag, msg) spdlog::error("{}: {}\n[file]: {} ({})\n[function]: {}", (tag), &(msg)[0], __FILE__, __LINE__, __FUNCTION__)
    #define LOG_WARN(tag, msg) spdlog::warn("{}: {}\n[file]: {} ({})\n[function]: {}", (tag), &(msg)[0], __FILE__, __LINE__, __FUNCTION__)
    #define LOG_INFO(tag, msg) spdlog::info("{}: {}\n[file]: {} ({})\n[function]: {}", (tag), &(msg)[0], __FILE__, __LINE__, __FUNCTION__)
#else
    #define LOG_ERROR(tag, msg)
    #define LOG_WARN(tag, msg)
    #define LOG_INFO(tag, msg)
#endif