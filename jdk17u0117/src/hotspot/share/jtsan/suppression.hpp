#ifndef SUPPRESSION_HPP
#define SUPPRESSION_HPP

#include "stacktrace.hpp"
#include "memory/allocation.hpp"

#include <unordered_map>

// Custom allocator for std::unordered_map

template <typename T>
class CustomPairAllocator {
public:
    using value_type = T;

    CustomPairAllocator() = default;

    template <typename U>
    constexpr CustomPairAllocator(const CustomPairAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) exit(1);
        if (auto p = static_cast<T*>(NEW_C_HEAP_ARRAY(T, n * sizeof(T), mtInternal))) {
            return p;
        }
        exit(1);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        FREE_C_HEAP_ARRAY(T, p);
    }
};

template <typename T, typename U>
bool operator==(const CustomPairAllocator<T>&, const CustomPairAllocator<U>&) { return true; }

template <typename T, typename U>
bool operator!=(const CustomPairAllocator<T>&, const CustomPairAllocator<U>&) { return false; }




// Trie data structure for string matching


class TrieNode : public CHeapObj<mtInternal> {
public:
    //std::unordered_map<char, TrieNode*> children;
    // use the custom allocator
    std::unordered_map<char, TrieNode*, std::hash<char>, std::equal_to<char>, 
        CustomPairAllocator<std::pair<const char, TrieNode*>>> children;
    bool is_end_of_word;
    bool has_wildcard;

    TrieNode() : is_end_of_word(false), has_wildcard(false) {}
};

class Trie : public CHeapObj<mtInternal> {
    private:
        TrieNode* root;

    public:
        Trie();

        void insert(const char *name);
        bool search(const char *name);
};

class JTSanSuppression : public CHeapObj<mtInternal> {
    public:
        static void init();
        static bool is_suppressed(JavaThread *thread);
    private:
        static Trie *top_frame_suppressions;
        static Trie *frame_suppressions;
};


#endif
