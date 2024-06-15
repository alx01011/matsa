#ifndef SUPPRESSION_HPP
#define SUPPRESSION_HPP

#include "stacktrace.hpp"
#include "pair_allocator.hpp"
#include "memory/allocation.hpp"

#include <unordered_map>

class TrieNode : public CHeapObj<mtInternal> {
public:
    //std::unordered_map<char, TrieNode*> children;
    // use the custom allocator
    std::unordered_map<char, TrieNode*, std::hash<char>, std::equal_to<char>, 
        CustomAllocator<std::pair<const char, TrieNode*>>> children;
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
        static bool is_suppressed(JTSanStackTrace *stack_trace);
    private:
        static Trie *top_frame_suppressions;
        static Trie *frame_suppressions;
};


#endif
