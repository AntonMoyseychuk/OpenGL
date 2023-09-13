#pragma once

#include <random>

template <typename Type, typename = std::enable_if_t<std::is_arithmetic_v<Type>>>
inline Type random(Type min, Type max) noexcept {       
    static std::random_device _rd;
    static std::mt19937 gen(_rd());
    
    if constexpr (std::is_floating_point_v<Type>) {
        return std::uniform_real_distribution<Type>(min, max)(gen);
    } else {
        return std::uniform_int_distribution<Type>(min, max)(gen);
    }
}