#pragma once

struct nocopyable {
    nocopyable() = default;
    
    nocopyable(nocopyable&& nocopyable) noexcept = default;
    nocopyable& operator=(nocopyable&& nocopyable) noexcept = default;

    nocopyable(const nocopyable& nocopyable) = delete;
    nocopyable& operator=(const nocopyable& nocopyable) = delete;
};