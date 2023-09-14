#include <glad/glad.h>

#include "buffer.hpp"
#include "debug.hpp"

buffer::buffer(int32_t target, size_t size, size_t element_size, int32_t usage, const void* data) {
    create(target, size, element_size, usage, data);
}

buffer::~buffer() {
    destroy();
}

void buffer::create(int32_t target, size_t size, size_t element_size, int32_t usage, const void* data) noexcept {
    if (id != 0) {
        LOG_WARN(_target_to_string(target) + " buffer", _target_to_string(target) + "buffer recreation (prev id = " + std::to_string(id) + ")");
        destroy();
    }

    this->size = size;
    this->element_size = element_size;
    this->target = target;
    this->usage = usage;

    OGL_CALL(glGenBuffers(1, &id));
    bind();
    OGL_CALL(glBufferData(target, size, data, usage));
}

void buffer::destroy() noexcept {
    OGL_CALL(glDeleteBuffers(1, &id));
    id = 0;
}

void buffer::subdata(uint32_t offset, size_t size, const void* data) const noexcept {
    bind();
    OGL_CALL(glBufferSubData(target, offset, size, data));
}

void buffer::bind() const noexcept {
    OGL_CALL(glBindBuffer(target, id));
}

void buffer::unbind() const noexcept {
    OGL_CALL(glBindBuffer(target, 0));
}

size_t buffer::get_element_count() const noexcept {
    if (element_size == 0) {
        return 0;
    }

    ASSERT(size % element_size == 0, "buffer", "remainder of size divided by element_size is not zero");

    return size / element_size;
}

buffer::buffer(buffer&& buffer) noexcept
    : size(buffer.size), element_size(buffer.element_size), target(buffer.target), usage(buffer.usage), id(buffer.id)
{
    if (this != &buffer) {
        memset(&buffer, 0, sizeof(buffer));
    }
}

buffer& buffer::operator=(buffer&& buffer) noexcept {
    if (this != &buffer) {
        size = buffer.size;
        element_size = buffer.element_size;
        target = buffer.target;
        usage = buffer.usage;
        id = buffer.id;

        memset(&buffer, 0, sizeof(buffer));
    }

    return *this;
}

const std::string& buffer::_target_to_string(uint32_t target) noexcept {
    static const std::unordered_map<uint32_t, std::string> strings = {
        std::make_pair(GL_ARRAY_BUFFER, "vertex"),
        std::make_pair(GL_ELEMENT_ARRAY_BUFFER, "index"),
        std::make_pair(GL_UNIFORM_BUFFER, "uniform")
    };

    static const std::string unrecognized = "unrecognized";

    return strings.find(target) != strings.cend() ? strings.at(target) : unrecognized;
}
