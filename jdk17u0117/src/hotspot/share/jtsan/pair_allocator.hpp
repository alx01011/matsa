#ifndef PAIR_ALLOCATOR_HPP
#define PAIR_ALLOCATOR_HPP

#include <cstdlib>
#include <stdio.h>

template <typename T>
class CustomAllocator {
public:
    using value_type = T;

    CustomAllocator() = default;

    template <typename U>
    constexpr CustomAllocator(const CustomAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) exit(1);
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
            return p;
        }
        exit(1);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        std::free(p);
    }
};

template <typename T, typename U>
bool operator==(const CustomAllocator<T>&, const CustomAllocator<U>&) { return true; }

template <typename T, typename U>
bool operator!=(const CustomAllocator<T>&, const CustomAllocator<U>&) { return false; }

#endif