#pragma once
#include <vector>
#include <string>

class cubemap {
public:
    cubemap() = default;
    cubemap(const std::vector<std::string>& faces);
    ~cubemap();

    void create(const std::vector<std::string>& faces) noexcept;
    void destroy() noexcept;

    void bind(uint32_t unit = 0) const noexcept;
    void unbind() const noexcept;

    cubemap(cubemap&& cubemap);
    cubemap& operator=(cubemap&& cubemap) noexcept;

    cubemap(const cubemap& cubemap) = delete;
    cubemap& operator=(const cubemap& cubemap) = delete;

private:
    uint32_t m_id = 0;
};