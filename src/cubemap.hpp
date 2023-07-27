#pragma once
#include <vector>
#include <string>

class cubemap {
public:
    cubemap() = default;
    cubemap(const std::vector<std::string>& faces);

    void create(const std::vector<std::string>& faces) noexcept;

    void bind() const noexcept;
    void unbind() const noexcept;

private:
    uint32_t m_id;
};