#pragma once
#include "assert.hpp"

#include <string>
#include <debugbreak.h>

namespace detail {
    const char* gl_error_description(uint32_t err) noexcept;
    void gl_clear_errors() noexcept;
}

#ifdef _DEBUG
    #define OGL_CALL(gl_call) do { \
        detail::gl_clear_errors(); \
        gl_call; \
        const auto err = glGetError(); \
        ASSERT(err == GL_NO_ERROR, "OpenGL error", std::string(#gl_call) + " " + detail::gl_error_description(err)); \
    } while(0)
#else
    #define OGL_CALL(gl_call) gl_call
#endif